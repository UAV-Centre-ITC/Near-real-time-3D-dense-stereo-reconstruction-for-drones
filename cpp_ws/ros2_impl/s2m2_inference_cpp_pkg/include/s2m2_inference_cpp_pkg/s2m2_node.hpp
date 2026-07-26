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
#include <Eigen/Geometry>
#include <chrono>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cv_bridge/cv_bridge.h>
#include <fstream>
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
#include <thread>

/**
 * @brief ROS2 node that performs disparity inference using TensorRT,
 *        reprojects to 3D points, transforms them to world coordinates,
 *        filters invalid points, and publishes the resulting point cloud.
 *
 * The node loads a set of pre-captured stereo image pairs, rectifies them,
 * runs the S2M2 TensorRT model for disparity estimation, and produces a
 * filtered 3D point cloud in the world coordinate frame. Processing is
 * orchestrated by startProcessing() and is triggered once via a startup
 * timer. Multiple stereo pairs (image pairs 759/760, 760/761, 761/762)
 * are processed sequentially.
 */
class S2M2Node : public rclcpp::Node {
public:
  /**
   * @brief Constructs the S2M2Node, initializing ROS2 parameters,
   *        the TensorRT engine, CUDA resources, publishers, and the
   *        startup timer that triggers startProcessing().
   */
  S2M2Node();

  /**
   * @brief Destructor, cleans up the TensorRT engine, execution context,
   *        runtime, and CUDA events.
   */
  ~S2M2Node();

  /**
   * @brief Loads and allocates CUDA buffers by querying the TensorRT engine
   *        for input/output tensor sizes. Allocates 5 buffers for
   *        {input_left, input_right, output_disp, output_occ, output_conf}
   *        plus an extra buffer for the 3D point cloud.
   */
  void loadCudaBuffers();

  /**
   * @brief Pads the rectified stereo pair to dimensions that are multiples
   *        of 32 and converts the images from HWC to NCHW layout.
   * @param left_rectified  Left rectified image.
   * @param right_rectified Right rectified image.
   * @return A vector containing the processed left and right images
   *         (padded, in NCHW format).
   */
  [[nodiscard]] std::vector<cv::Mat> preprocessInputs(cv::Mat &left_rectified,
                                                      cv::Mat &right_rectified);

  /**
   * @brief Resizes the input images using scale_factor_, rectifies them
   *        using the provided extrinsics, and optionally saves the
   *        rectified images to disk.
   * @param left_image  Original left camera image.
   * @param right_image Original right camera image.
   * @param extrinsics  Extrinsic parameters for stereo rectification.
   */
  void rectifyImages(cv::Mat &left_image, cv::Mat &right_image,
                     const StereoExtrinsics &extrinsics);

  /**
   * @brief Copies the preprocessed stereo pair to GPU, runs TensorRT
   *        inference, and records GPU timing.
   * @param processed_pair A vector of [left, right] images in NCHW format
   *                       (output of preprocessInputs).
   * @return A vector of GPU Mats: {disparity, occlusion, confidence}.
   */
  [[nodiscard]] std::vector<cv::cuda::GpuMat>
  runInference(std::vector<cv::Mat> &processed_pair);

  /**
   * @brief Reprojects the disparity map to a homogeneous 3D point cloud
   *        using the Q matrix from rectification.
   * @param disp    GPU disparity map.
   * @param Qmatrix 4x4 reprojection matrix (CV_32FC1).
   * @return GPU Mat containing the homogeneous 3D points (CV_32FC4).
   */
  [[nodiscard]] cv::cuda::GpuMat runReprojectionTo3D(cv::cuda::GpuMat &disp,
                                                     cv::Mat &Qmatrix);

  /**
   * @brief Transforms the 3D point cloud from the rectified left camera
   *        frame to the world coordinate frame using the provided
   *        transformation matrix.
   * @param points3d_homog_gpu Homogeneous 3D points in camera frame.
   * @param T_WC               4x4 transformation matrix (camera→world).
   * @return GPU Mat containing the transformed 3D points (CV_32FC4).
   */
  [[nodiscard]] cv::cuda::GpuMat
  runTransformationWC(cv::cuda::GpuMat &points3d_homog_gpu,
                      const TransformMatrix &T_WC);

  /**
   * @brief Computes the 4x4 transformation matrix that maps points from
   *        the rectified left camera frame to the world coordinate frame.
   *        The transform combines the inverse of the rectification rotation
   *        with the inverse of the camera-to-world extrinsic rotation.
   * @param extrinsics Extrinsic parameters of the left camera.
   * @return The homogeneous 4x4 transformation matrix (camera→world).
   */
  [[nodiscard]] TransformMatrix
  generateTransformWC(const StereoExtrinsics &extrinsics);

  /**
   * @brief Downloads the transformed 3D point cloud from GPU to CPU,
   *        publishes the left processed image and the point cloud as
   *        ROS2 messages, and optionally saves/visualizes the disparity.
   * @param transformed_points3d Transformed point cloud on GPU.
   * @param confidence           Confidence buffer from inference.
   * @param occlusion            Occlusion buffer from inference.
   */
  void publishMessages(const cv::cuda::GpuMat &transformed_points3d,
                       const cv::cuda::GpuMat &confidence,
                       const cv::cuda::GpuMat &occlusion);

  /**
   * @brief Converts an image from OpenCV HWC (height, width, channels)
   *        layout to TensorRT NCHW layout by splitting the channels
   *        and concatenating them vertically.
   * @param img Input/output image modified in-place.
   */
  void convertToNCHW(cv::Mat &img);

  /**
   * @brief Filters the 3D point cloud by zeroing out points with low
   *        confidence or high occlusion values according to the configured
   *        thresholds.
   * @param points3d  3D point cloud modified in-place on GPU.
   * @param confidence Confidence values from the S2M2 model.
   * @param occlusion  Occlusion values from the S2M2 model.
   * @param stream     CUDA stream for asynchronous execution.
   */
  void filterPoints3D(cv::cuda::GpuMat &points3d, cv::cuda::GpuMat &confidence,
                      cv::cuda::GpuMat &occlusion, cv::cuda::Stream &stream);

  /**
   * @brief Orchestrates the full processing pipeline for all three stereo
   *        pairs: rectification, preprocessing, inference, reprojection,
   *        coordinate transformation, filtering, and publishing. Timing
   *        for each pair is recorded and written to the profiler file.
   */
  void startProcessing();

private:
  rclcpp::TimerBase::SharedPtr startup_timer_;
  StereoRectifier rectifier_;

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
  sensor_msgs::msg::Image left_image_msg_;
  std::vector<float> disparity_out_buffer_;
  std::vector<float> confidence_out_buffer_;
  std::vector<float> occlusion_out_buffer_;
  int new_height_;
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
