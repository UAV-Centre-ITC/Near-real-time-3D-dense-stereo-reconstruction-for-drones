#pragma once
#include "calibration_data.hpp"
#include "cam_params.hpp"
#include "inference_buffer.hpp"
#include "logger.hpp"
#include "s2m2_inference_cpp_pkg/cuda_kernels.h"
#include "stereo_rectifier.hpp"
#include "trt_file_reader.hpp"
#include "unsupported/Eigen/CXX11/Tensor"
#include "utils/image_helpers.hpp"
#include <cassert>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cv_bridge/cv_bridge.h>
#include <fstream>
#include <map>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda_stream_accessor.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <string>

class S2M2Node : public rclcpp::Node {
public:
  /**
   * @brief Constructs the S2M2Node, initializing ROS2 parameters, TensorRT
   * engine, CUDA resources, publishers, subscribers, and the processing worker
   * thread.
   */
  S2M2Node();

  /**
   * @brief Destructor, cleans up TensorRT engine, CUDA events, and joins worker
   * thread.
   */
  ~S2M2Node();

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

  void rectifyFullResolutionImages(cv::Mat &left_image, cv::Mat &right_image,
                                   const StereoExtrinsics &extrinsics);

  /**
   * @brief Executes TensorRT inference on preprocessed stereo images,
   * reprojects disparity to 3D points, transforms to world frame, and publishes
   * results.
   */
  [[nodiscard]] std::vector<cv::cuda::GpuMat>
  runInference(std::vector<cv::Mat> &processed_pair);

  [[nodiscard]] cv::cuda::GpuMat runReprojectionTo3D(cv::cuda::GpuMat &disp,
                                                     cv::Mat &Qmatrix,
                                                     int offset_x,
                                                     int offset_y);

  [[nodiscard]] cv::cuda::GpuMat
  runTransformationWC(cv::cuda::GpuMat &points3d_homog_gpu,
                      const StereoExtrinsics &extrinsics);

  [[nodiscard]] std::map<std::string, cv::Rect>
  splitImageTo4Patches(cv::Mat &original_img);

  void publishMessages(const cv::cuda::GpuMat &points3d_homog_transformed_gpu);

  /**
   * @brief Converts image from OpenCV HWC format to TensorRT NCHW format.
   * @param img Input/output cv::Mat image, transformed in-place.
   */
  void convertToNCHW(cv::Mat &img);

  /**
   * @brief Calculates and stores the transform need to take the 3D points
   * from the rectified left camera frame to the world frame. It first undoes
   * the rotation produced through stereoRectify(), and then applies the inverse
   * of the CW transform that is given through the R,t of pix4d extrinsics.
   * @param  extrinsics Extrinsic parameters of the left camera.
   * world coordinates.
   */
  [[nodiscard]] TransformMatrix
  generateTransformWC(const StereoExtrinsics &extrinsics);

  /**
   * @brief Filters 3D points by zeroing out invalid points based on confidence
   * and occlusion thresholds.
   * @param points3d The homogeneous 3D point cloud (modified in-place on GPU).
   * @param confidence Confidence values from the S2M2 network.
   * @param occlusion Occlusion values from the S2M2 network.
   * @param stream CUDA stream for asynchronous execution.
   */
  void filterPoints3D(cv::cuda::GpuMat &points3d, cv::cuda::GpuMat &confidence,
                      cv::cuda::GpuMat &occlusion, cv::cuda::Stream &stream);

  /**
   * @brief Processes all three stereo pairs sequentially: preprocess, infer,
   * transform, filter, and publish results.
   */
  void startProcessing();

private:
  rclcpp::TimerBase::SharedPtr startup_timer_;
  StereoRectifier rectifier_;

  bool current_frame_is_left_;
  bool is_first_frame_ = true;

  /* CUDA and TensorRT */
  std::string engine_path_;
  nvinfer1::IRuntime *runtime_;
  nvinfer1::ICudaEngine *engine_;
  nvinfer1::IExecutionContext *context_;
  cudaEvent_t start_inference_;
  cudaEvent_t end_inference_;
  cudaEvent_t start_reproject_;
  cudaEvent_t end_reproject_;
  cudaEvent_t start_transform_kernel_;
  cudaEvent_t end_transform_kernel_;
  cudaEvent_t start_filter_;
  cudaEvent_t end_filter_;

  std::vector<CudaBuffer> buffers_;
  cudaStream_t stream_;

  // Image processing
  cv::Mat image_759_;
  cv::Mat image_760_;
  cv::Mat image_761_;
  cv::Mat image_762_;
  cv::Mat left_image_rectified_;
  cv::Mat right_image_rectified_;
  sensor_msgs::msg::Image left_image_msg_; // for coloring the pointcloud
  int model_input_height_;
  int model_input_width_;
  int new_height_; // of padded resized images
  int new_width_;
  // Time profiling
  std::string profiler_dirpath_;
  std::ofstream profiler_file_;
  int profiler_writes_count_ = 0;
  int disp_save_counter_ = 0;
  // Pub/subs :
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr points3d_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr left_processed_pub_;

  double scale_factor_;
  double occlusion_threshold_;
  double confidence_threshold_;
};
