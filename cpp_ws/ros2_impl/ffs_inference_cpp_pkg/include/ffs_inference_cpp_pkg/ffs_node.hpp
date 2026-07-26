#pragma once
#include "calibration_data.hpp"
#include "cam_params_rosbag.hpp"
#include "ffs_inference_cpp_pkg/cuda_kernels.h"
#include "inference_buffer.hpp"
#include "logger.hpp"
#include "stereo_rectifier.hpp"
#include "trt_file_reader.hpp"
#include "unsupported/Eigen/CXX11/Tensor"
#include "utils/image_helpers.hpp"
#include <Eigen/Geometry> // for Quaternion and rotation() and translation() helpers to convert.
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
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <string>
#include <tf2_eigen/tf2_eigen.hpp>
#include <thread>

enum class STEREO_SETUP_NOW {
  NO_KEYFRAME,
  RESET_KEYFRAME,
  HORIZONTAL_KEYFRAME
};

// The relative R,T that bring camera 1(left) to camera 2(right) coordinate
// frame (!)
struct TransformComponents {
  cv::Mat R_rel;
  cv::Mat t_rel;
};

class FFSNode : public rclcpp::Node {
public:
  /**
   * @brief Constructs the FFSNode, initializing ROS2 parameters, TensorRT
   * engine, CUDA resources, publishers, subscribers, and the processing worker
   * thread.
   */
  FFSNode();

  /**
   * @brief Destructor, cleans up TensorRT engine, CUDA events, and joins worker
   * thread.
   */
  ~FFSNode();

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
   * @param current_pose_msg_copy The absolute pose message of the current
   * frame.
   * @return STEREO_SETUP_NOW indicating: NO_KEYFRAME (insignificant motion),
   * RESET_KEYFRAME (significant rotation/Z motion, reset required),
   * HORIZONTAL_KEYFRAME (valid stereo pair with horizontal motion).
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

  [[nodiscard]] std::map<std::string, cv::Rect>
  splitImageTo4Patches(cv::Mat &original_img);

  /**
   * @brief Loads and allocates CUDA buffers for TensorRT input/output tensors.
   */
  void loadCudaBuffers();

  /**
   * @brief Preprocesses stereo image pair: rectification, resizing, padding,
   * and NCHW conversion.
   * @param left_image Input left camera image (cv::Mat).
   * @param right_image Input right camera image (cv::Mat).
   */
  [[nodiscard]] std::vector<cv::Mat> preprocessInputs(cv::Mat &left_image,
                                                      cv::Mat &right_image);

  /**
   * @brief Executes TensorRT inference on preprocessed stereo images,
   * reprojects disparity to 3D points, transforms to world frame, and publishes
   * results.
   */
  [[nodiscard]] cv::cuda::GpuMat
  runInference(std::vector<cv::Mat> &processed_pair);

  /**
   * @brief Converts image from OpenCV HWC format to TensorRT NCHW format, and
   * also normalizes the images with the ImageNet mean, necessary for the
   * FastFoundation stereo model.
   * @param img Input/output cv::Mat image, transformed in-place.
   */
  void normalizeAndConvertToNCHW(cv::Mat &img);

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
   * @brief Stores the absolute pose of the left camera for world-frame
   * transformation.
   * @param absolute_pose_msg PoseStamped message containing camera pose in
   * world coordinates.
   */
  [[nodiscard]] TransformMatrix generateTransformWC(
      geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg);

  /**
   * @brief Worker thread that processes incoming keyframe pairs sequentially.
   * Waits for new data, performs preprocessing, inference, and publishing.
   */
  void processingThread();

  void processValidPair(
      cv::Mat &left_image, cv::Mat &right_image,
      geometry_msgs::msg::PoseStamped::ConstSharedPtr current_keyframe_pose);

  void rectifyFullResolutionImages(cv::Mat &left_image, cv::Mat &right_image);

  [[nodiscard]] cv::cuda::GpuMat runReprojectionTo3D(cv::cuda::GpuMat &disp_gpu,
                                                     cv::Mat &Qmatrix,
                                                     const int patch_offset_x,
                                                     const int patch_offset_y);

  [[nodiscard]] cv::cuda::GpuMat
  runTransformationWC(cv::cuda::GpuMat &points3d_homog_gpu,
                      TransformMatrix &T_WC);

  void publishMessages(const cv::cuda::GpuMat &points3d_transformed_gpu);

private:
  using ApproximateTimeImagePose =
      message_filters::sync_policies::ApproximateTime<
          sensor_msgs::msg::Image, geometry_msgs::msg::PoseStamped>;
  rclcpp::TimerBase::SharedPtr timer_;
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
  cv::Mat points3d_homog_;
  cv::cuda::GpuMat points3d_homog_gpu_;
  cv::cuda::GpuMat points3d_transformed_homog_gpu_;
  sensor_msgs::msg::Image left_image_msg_; // for coloring the pointcloud
  std::vector<float> disparity_out_buffer_;
  int new_height_; // of padded resized images
  int new_width_;
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
  double max_keyframe_Z_;
  double occlusion_threshold_;
  double confidence_threshold_;
};
