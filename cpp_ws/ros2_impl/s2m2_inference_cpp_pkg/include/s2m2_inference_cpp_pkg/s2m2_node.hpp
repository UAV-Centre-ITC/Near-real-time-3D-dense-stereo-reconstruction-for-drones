#pragma once
#include "calibration_data.hpp"
#include "cam_params_rosbag.hpp"
#include "inference_buffer.hpp"
#include "logger.hpp"
#include "s2m2_inference_cpp_pkg/cuda_kernels.h"
#include "stereo_rectifier.hpp"
#include "trt_file_reader.hpp"
#include "unsupported/Eigen/CXX11/Tensor"
#include "utils/image_helpers.hpp"
#include <Eigen/Geometry>
#include <chrono>
#include <condition_variable>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cv_bridge/cv_bridge.h>
#include <fstream>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <image_transport/image_transport.hpp>
#include <image_transport/subscriber_filter.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.hpp>
#include <message_filters/synchronizer.hpp>
#include <mutex>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda_stream_accessor.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sstream>
#include <string>
#include <tf2_eigen/tf2_eigen.hpp>
#include <thread>

namespace s2m2_inference_cpp_pkg {

/**
 * @brief Outcome of keyframe evaluation for a new incoming frame.
 * NO_KEYFRAME: insufficient motion to form a stereo pair.
 * RESET_KEYFRAME: excessive rotation or Z displacement, discard previous
 * keyframe and restart.
 * VALID_PAIR: acceptable horizontal motion — current and previous frames form a
 * stereo pair.
 */
enum class STEREO_SETUP_NOW { NO_KEYFRAME, RESET_KEYFRAME, VALID_PAIR };

// The relative R,T that bring camera 1(left) to camera 2(right) coordinate
// frame (!)
struct TransformComponents {
  cv::Mat R_rel;
  cv::Mat t_rel;
};

// Per-pass profiling samples accumulated during a single valid pair run.
// Helpers write into these fields instead of writing directly to disk; the
// whole struct is flushed once at the end of processValidPair().
struct PassTimings {
  double padding_ms = 0.0;
  double nchw_ms = 0.0;
  double inference_ms = 0.0;
  double reproject_ms = 0.0;
  double transform_ms = 0.0;
  double filter_ms = 0.0;
  double download_publish_ms = 0.0;
};

// Per-pair profiling samples (rectification is done once per pair, the rest
// is captured once for the full resolution image).
struct PairTimings {
  double rectify_left_ms = 0.0;
  double rectify_right_ms = 0.0;
  double rectify_total_ms = 0.0;
  PassTimings pass{};
  double pair_total_ms = 0.0;
};

class S2M2Node : public rclcpp::Node {
public:
  /**
   * @brief Constructs the S2M2Node, initializing ROS2 parameters, TensorRT
   * engine, CUDA resources, publishers, subscribers, and the processing worker
   * thread.
   */
  S2M2Node(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

  /**
   * @brief Destructor, cleans up TensorRT engine, CUDA events, and joins worker
   * thread.
   */
  ~S2M2Node();

  /**
   * @brief Synchronized callback for incoming image and pose messages.
   * @param left_image Incoming camera image message.
   * @param absolute_pose_msg Absolute pose of the current frame in world
   * coordinates.
   */
  void syncCallback(
      const sensor_msgs::msg::Image::ConstSharedPtr &left_image,
      const geometry_msgs::msg::PoseStamped::ConstSharedPtr &absolute_pose_msg);

  /**
   * @brief Determines whether the current frame qualifies as a keyframe based
   * on pose changes.
   * @return STEREO_SETUP_NOW indicating: NO_KEYFRAME (insignificant motion),
   * RESET_KEYFRAME (significant rotation/Z motion, reset required),
   * VALID_PAIR (valid stereo pair with horizontal motion).
   */
  [[nodiscard]] STEREO_SETUP_NOW evaluateKeyframe(
      geometry_msgs::msg::PoseStamped::ConstSharedPtr current_pose_msg_copy);

  /**
   * @brief Extracts relative rotation and translation components from a
   * transform.
   * @param transform Eigen Isometry3d transform matrix.
   * @return TransformComponents containing relative rotation (R_rel) and
   * translation (t_rel).
   */
  [[nodiscard]] TransformComponents
  extractTransformComponents(Eigen::Isometry3d transform);

  /**
   * @brief Filters the transformed 3D points by confidence and occlusion
   * thresholds. Points that fail either threshold are set to zero in-place on
   * the GPU.
   * @param confidence Confidence map from the S2M2 model.
   * @param occlusion Occlusion map from the S2M2 model.
   * @param stream OpenCV CUDA stream for asynchronous operation.
   * @param pass_t Output: per-pass timing samples.
   */
  void filterPoints3D(cv::cuda::GpuMat &confidence, cv::cuda::GpuMat &occlusion,
                      cv::cuda::Stream &stream, PassTimings &pass_t);

  /**
   * @brief Loads and allocates CUDA buffers for TensorRT input/output tensors.
   */
  void loadCudaBuffers();

  /**
   * @brief Preprocesses the stereo image pair for TensorRT input: resizes or
   * pads to model dimensions, converts from HWC to NCHW format.
   * Per-pass timings for padding and NCHW conversion are accumulated into
   * pass_t.
   * @param left_image Input left camera image (cv::Mat).
   * @param right_image Input right camera image (cv::Mat).
   * @param pass_t Output: per-pass timing samples.
   * @return Vector of two NCHW-formatted cv::Mat: [left_processed,
   * right_processed].
   */
  [[nodiscard]] std::vector<cv::Mat> preprocessInputs(cv::Mat &left_image,
                                                       cv::Mat &right_image,
                                                       PassTimings &pass_t);

  /**
   * @brief Executes TensorRT inference on the preprocessed stereo pair.
   * Copies input images to GPU buffers, enqueues the engine, and returns the
   * three output tensors (disparity, occlusion, confidence) as GPU mats.
   * Per-pass inference timing is accumulated into pass_t.
   * @param processed_pair NCHW-formatted left and right images.
   * @param pass_t Output: per-pass timing samples.
   * @return Vector of three GPU mats: [disp_gpu, occ_gpu, conf_gpu].
   */
  [[nodiscard]] std::vector<cv::cuda::GpuMat>
  runInference(std::vector<cv::Mat> &processed_pair, PassTimings &pass_t);

  /**
   * @brief Converts image from OpenCV HWC format to TensorRT NCHW format.
   * @param img Input/output cv::Mat image, transformed in-place.
   */
  void convertToNCHW(cv::Mat &img);

  /**
   * @brief Assigns left and right images based on which frame was captured
   * first.
   * @param current_image_msg The current keyframe image message.
   * @param left_image Out: assigned left image (cv::Mat).
   * @param right_image Out: assigned right image (cv::Mat).
   */
  void
  setLeftRightImages(sensor_msgs::msg::Image::ConstSharedPtr current_image_msg,
                     cv::Mat &left_image, cv::Mat &right_image);

  /**
   * @brief Generates a 4x4 World-to-Camera transform matrix from the absolute
   * pose of the left camera, accounting for rectification rotation.
   * @param absolute_pose_msg PoseStamped message containing camera pose in
   * world coordinates.
   * @return TransformMatrix T_WC (row-major, 16 floats) mapping world-frame
   * points into the rectified left-camera frame.
   */
  [[nodiscard]] TransformMatrix generateTransformWC(
      geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg);

  /**
   * @brief Worker thread that processes incoming keyframe pairs sequentially.
   * Waits for new data, performs preprocessing, inference, and publishing.
   */
  void processingThread();

  /**
   * @brief Runs the full stereo pipeline on a validated keyframe pair.
   * Rectification, preprocessing, inference, reprojection, world-frame
   * transformation, filtering, and publishing.
   * @param left_image Raw left camera image.
   * @param right_image Raw right camera image.
   * @param current_keyframe_pose Pose of the current (more recent) keyframe.
   */
  void processValidPair(
      cv::Mat &left_image, cv::Mat &right_image,
      geometry_msgs::msg::PoseStamped::ConstSharedPtr current_keyframe_pose);

  /**
   * @brief Rectifies the full-resolution stereo pair using the relative
   * transformation between keyframes. Saves rectified images to disk if
   * configured.
   * @param left_image Left camera image (raw).
   * @param right_image Right camera image (raw).
   * @param timings Output: pair-level timing samples.
   */
  void rectifyFullResolutionImages(cv::Mat &left_image, cv::Mat &right_image,
                                   PairTimings &timings);

  /**
   * @brief Reprojects a disparity map to 3D homogeneous points on the GPU
   * using a custom CUDA kernel.
   * @param disp_gpu Disparity map (GPU mat, CV_32FC1).
   * @param Qmatrix Stereo Q-matrix (4x4, CV_32FC1).
   * @param patch_offset_x Horizontal offset of the current image patch (0 for
   * full-resolution).
   * @param patch_offset_y Vertical offset of the current image patch (0 for
   * full-resolution).
   * @param pass_t Output: per-pass timing samples.
   */
  void runReprojectionTo3D(cv::cuda::GpuMat &disp_gpu, const cv::Mat &Qmatrix,
                           const int patch_offset_x, const int patch_offset_y,
                           PassTimings &pass_t);

  /**
   * @brief Applies the world-to-camera transform to the 3D homogeneous points
   * on the GPU using a custom CUDA kernel.
   * @param T_WC 4x4 transform matrix (row-major, 16 floats).
   * @param pass_t Output: per-pass timing samples.
   */
  void runTransformationWC(const TransformMatrix &T_WC, PassTimings &pass_t);

  /**
   * @brief Downloads processed data from GPU and publishes the 3D point-cloud
   * and processed left-image messages.
   * @param pass_t Output: per-pass timing samples.
   */
  void publishMessages(PassTimings &pass_t);

private:
  using ApproximateTimeImagePose =
      message_filters::sync_policies::ApproximateTime<
          sensor_msgs::msg::Image, geometry_msgs::msg::PoseStamped>;
  StereoRectifier rectifier_;
  geometry_msgs::msg::PoseStamped::ConstSharedPtr
      last_keyframe_absolute_pose_msg_;
  geometry_msgs::msg::PoseStamped::ConstSharedPtr
      current_keyframe_absolute_pose_msg_;
  sensor_msgs::msg::Image::ConstSharedPtr last_keyframe_image_msg_;
  sensor_msgs::msg::Image::ConstSharedPtr current_keyframe_image_msg_;
  Eigen::Isometry3d T_previous_to_now_;
  TransformComponents relative_Rt_previous_to_current_;
  bool current_frame_is_left_;
  uint32_t left_rect_sec_;
  uint32_t left_rect_nanosec_;
  bool is_first_frame_ = true;

  /* CUDA and TensorRT */
  std::string engine_path_;
  nvinfer1::IRuntime *runtime_;
  nvinfer1::ICudaEngine *engine_;
  nvinfer1::IExecutionContext *context_;
  cudaEvent_t start_inference_;
  cudaEvent_t end_inference_;
  cudaEvent_t start_transform_kernel_;
  cudaEvent_t end_transform_kernel_;
  cudaEvent_t start_reproject_;
  cudaEvent_t end_reproject_;
  cudaEvent_t start_filter_;
  cudaEvent_t end_filter_;
  std::vector<CudaBuffer> buffers_;
  cudaStream_t stream_;

  // Image processing
  cv::Mat left_image_rectified_;
  cv::Mat right_image_rectified_;
  cv::Mat left_pass_processed_;
  cv::cuda::GpuMat points3d_homog_gpu_;
  cv::cuda::GpuMat points3d_transformed_homog_gpu_;
  int processed_img_height_;
  int processed_img_width_;
  // Time profiling
  std::string profiler_dirpath_;
  std::ofstream profiler_file_;
  int profiler_writes_count_ = 0;

  // Pub/subs :
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr points3d_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr left_processed_pub_;
  image_transport::SubscriberFilter image_sub_;
  message_filters::Subscriber<geometry_msgs::msg::PoseStamped>
      absolute_pose_sub_;
  std::shared_ptr<message_filters::Synchronizer<ApproximateTimeImagePose>>
      synchronizer_;

  // Worker thread :
  std::thread worker_thread_;
  bool keep_running_ = true; // use inside the mutex to avoid races
  bool new_data_ = false;    // use inside the mutex to avoid races
  std::condition_variable condition_var_;
  std::mutex mtx_;

  // param values :
  int model_input_width_;
  int model_input_height_;
  double min_keyframe_movement_;
  double max_keyframe_angular_;
  double max_keyframe_dZ_;
  double occlusion_threshold_;
  double confidence_threshold_;
};

} // namespace s2m2_inference_cpp_pkg
