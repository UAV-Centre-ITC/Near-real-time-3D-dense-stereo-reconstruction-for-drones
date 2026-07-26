#include "s2m2_inference_cpp_pkg/s2m2_node.hpp"
#include "s2m2_inference_cpp_pkg/cuda_kernels.h"
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <opencv2/core/types.hpp>
#include <ratio>
#include <rclcpp/duration.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/logging.hpp>

#define DEBUG_MODE true
#define HOMOGENEOUS_DIMS 4
#define NON_HOMOGENEOUS_DIMS 3

S2M2Node::S2M2Node() : Node("s2m2_node"), rectifier_{STEREO_INTRINSICS, false} {

#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "S2M2Node constructor");
#endif
  auto start_total = this->get_clock()->now();
  // Params :
  this->declare_parameter("engine_filepath", "");
  this->declare_parameter("image759_path", "");
  this->declare_parameter("image760_path", "");
  this->declare_parameter("image761_path", "");
  this->declare_parameter("image762_path", "");
  this->declare_parameter("model_input_height", 0);
  this->declare_parameter("model_input_width", 0);
  this->declare_parameter("save_disparity", false);
  this->declare_parameter("save_rectified_images", false);
  this->declare_parameter("rectified_images_dirpath", "");
  this->declare_parameter("visualize_disparity", false);
  this->declare_parameter("time_profiling_dirpath", "");
  this->declare_parameter("occlusion_threshold", 0.5);
  this->declare_parameter("confidence_threshold", 0.1);

  // Get params :
  engine_path_ = this->get_parameter("engine_filepath").as_string();
  model_input_height_ = this->get_parameter("model_input_height").as_int();
  model_input_width_ = this->get_parameter("model_input_width").as_int();
  profiler_dirpath_ = this->get_parameter("time_profiling_dirpath").as_string();
  profiler_file_ =
      std::ofstream(profiler_dirpath_ + "profiler_s2m2node.txt", std::ios::out);
  confidence_threshold_ =
      this->get_parameter("confidence_threshold").as_double();
  occlusion_threshold_ = this->get_parameter("occlusion_threshold").as_double();
  // TensorRT:
  FileStreamReader reader(engine_path_);
  TensorRTLogger logger;
  runtime_ = nvinfer1::createInferRuntime(logger);
  engine_ = runtime_->deserializeCudaEngine(reader);
  context_ = engine_->createExecutionContext();
  cudaStreamCreate(&stream_);
  cudaEventCreate(&start_inference_);
  cudaEventCreate(&end_inference_);
  cudaEventCreate(&start_transform_kernel_);
  cudaEventCreate(&end_transform_kernel_);
  cudaEventCreate(&start_reproject_);
  cudaEventCreate(&end_reproject_);
  cudaEventCreate(&start_filter_);
  cudaEventCreate(&end_filter_);

  // Publishers :
  points3d_pub_ =
      this->create_publisher<sensor_msgs::msg::Image>("stereo/points3d", 10);
  left_processed_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
      "stereo/left/processed", 10);
  // prealocate for in/out cuda buffers
  buffers_.reserve(
      5); // 5 buffers for input left and right, output disparity, occ, conf
  loadCudaBuffers();

  // read and store the images :
  image_759_ = cv::imread(this->get_parameter("image759_path").as_string(),
                          cv::IMREAD_COLOR);
  image_760_ = cv::imread(this->get_parameter("image760_path").as_string(),
                          cv::IMREAD_COLOR);
  image_761_ = cv::imread(this->get_parameter("image761_path").as_string(),
                          cv::IMREAD_COLOR);
  image_762_ = cv::imread(this->get_parameter("image762_path").as_string(),
                          cv::IMREAD_COLOR);
  // debug print their sizes :
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "image_759_ size: %d x %d", image_759_.cols,
              image_759_.rows);
  RCLCPP_INFO(this->get_logger(), "image_760_ size: %d x %d", image_760_.cols,
              image_760_.rows);
  RCLCPP_INFO(this->get_logger(), "image_761_ size: %d x %d", image_761_.cols,
              image_761_.rows);
  RCLCPP_INFO(this->get_logger(), "image_762_ size: %d x %d", image_762_.cols,
              image_762_.rows);
#endif
  // Start processing after 2 seconds:
  startup_timer_ = this->create_wall_timer(std::chrono::seconds(4), [this]() {
    startProcessing();
    startup_timer_->cancel();
  });
}

S2M2Node::~S2M2Node() {
  // cleanup::
  delete context_;
  delete engine_;
  delete runtime_;
  cudaEventDestroy(start_transform_kernel_);
  cudaEventDestroy(end_transform_kernel_);
  cudaEventDestroy(start_inference_);
  cudaEventDestroy(end_inference_);
  cudaEventDestroy(start_reproject_);
  cudaEventDestroy(end_reproject_);
  cudaEventDestroy(start_filter_);
  cudaEventDestroy(end_filter_);
}

void S2M2Node::startProcessing() {
  //------------------------ PAIR 1 -------------------------------
  auto start_pair1 = std::chrono::high_resolution_clock::now();
  rectifyFullResolutionImages(image_760_, image_759_, STEREO_EXTRINSICS_1);
  std::map patches_map_left = splitImageTo4Patches(left_image_rectified_);
  std::map patches_map_right = splitImageTo4Patches(right_image_rectified_);
  std::vector<cv::Mat> processed_pair_760_759;
  std::vector<cv::cuda::GpuMat> s2m2_output_buffers; // disp, occ , conf
  for (const auto &[name, roi] : patches_map_left) {
    // Get the left and right image patches and verify sizes:
    cv::Mat left_image_patch = left_image_rectified_(roi);
    cv::Mat right_image_patch = right_image_rectified_(roi);
    assert(left_image_patch.size() == right_image_patch.size());
    assert(left_image_patch.cols == model_input_width_);
    assert(left_image_patch.rows == model_input_height_);

    // Preprocess the pair of patches:
    processed_pair_760_759 =
        preprocessInputs(left_image_patch, right_image_patch);

    // Run s2m2 disparity inference on the processed pair:
    s2m2_output_buffers = runInference(processed_pair_760_759);

    // Reproject disparity to 3D points:
    int pixel_offset_x =
        (name == "top_left" || name == "bottom_left") ? 0 : roi.width;
    int pixel_offset_y =
        (name == "top_left" || name == "top_right") ? 0 : roi.height;
    cv::cuda::GpuMat &disp_gpu = s2m2_output_buffers[0];
    cv::Mat Qmatrix = rectifier_.getQMatrix();
    Qmatrix.convertTo(Qmatrix, CV_32FC1);
    cv::cuda::GpuMat points3d_homog_gpu =
        runReprojectionTo3D(disp_gpu, Qmatrix, pixel_offset_x, pixel_offset_y);

    // Transform points3d to world coordinate system:
    cv::cuda::GpuMat transformed_points3d_gpu =
        runTransformationWC(points3d_homog_gpu, STEREO_EXTRINSICS_1);

    // Filter points3d based on confidence and occlusion:
    cv::cuda::GpuMat &occlusion_gpu = s2m2_output_buffers[1];
    cv::cuda::GpuMat confidence_gpu = s2m2_output_buffers[2];
    cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
    filterPoints3D(transformed_points3d_gpu, confidence_gpu, occlusion_gpu,
                   cv_stream);

    // Publish the filtered points3d along with the (already made) left image:
    publishMessages(transformed_points3d_gpu);
  }
  auto end_pair1 = std::chrono::high_resolution_clock::now();
  auto dur_ms_pair1 =
      std::chrono::duration<double, std::milli>(end_pair1 - start_pair1)
          .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Total duration pair 1: %.3f ms",
              dur_ms_pair1);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Pair 1 duration: " << dur_ms_pair1 << "ms" << std::endl;
  }
  //------------------------ PAIR 2 -------------------------------
  auto start_pair2 = std::chrono::high_resolution_clock::now();
  rectifyFullResolutionImages(image_761_, image_760_, STEREO_EXTRINSICS_2);
  patches_map_left = splitImageTo4Patches(left_image_rectified_);
  patches_map_right = splitImageTo4Patches(right_image_rectified_);
  for (const auto &[name, roi] : patches_map_left) {
    cv::Mat left_image_patch = left_image_rectified_(roi);
    cv::Mat right_image_patch = right_image_rectified_(roi);
    assert(left_image_patch.size() == right_image_patch.size());
    assert(left_image_patch.cols == model_input_width_);
    assert(left_image_patch.rows == model_input_height_);

    processed_pair_760_759 =
        preprocessInputs(left_image_patch, right_image_patch);
    s2m2_output_buffers = runInference(processed_pair_760_759);

    int pixel_offset_x =
        (name == "top_left" || name == "bottom_left") ? 0 : roi.width;
    int pixel_offset_y =
        (name == "top_left" || name == "top_right") ? 0 : roi.height;
    cv::cuda::GpuMat &disp_gpu = s2m2_output_buffers[0];
    cv::Mat Qmatrix = rectifier_.getQMatrix();
    Qmatrix.convertTo(Qmatrix, CV_32FC1);
    cv::cuda::GpuMat points3d_homog_gpu =
        runReprojectionTo3D(disp_gpu, Qmatrix, pixel_offset_x, pixel_offset_y);

    cv::cuda::GpuMat transformed_points3d_gpu =
        runTransformationWC(points3d_homog_gpu, STEREO_EXTRINSICS_2);

    cv::cuda::GpuMat &occlusion_gpu = s2m2_output_buffers[1];
    cv::cuda::GpuMat confidence_gpu = s2m2_output_buffers[2];
    cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
    filterPoints3D(transformed_points3d_gpu, confidence_gpu, occlusion_gpu,
                   cv_stream);

    publishMessages(transformed_points3d_gpu);
  }
  auto end_pair2 = std::chrono::high_resolution_clock::now();
  auto dur_ms_pair2 =
      std::chrono::duration<double, std::milli>(end_pair2 - start_pair2)
          .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Total duration pair 2: %.3f ms",
              dur_ms_pair2);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Pair 2 duration: " << dur_ms_pair2 << "ms" << std::endl;
  }
  //------------------------ PAIR 3 -------------------------------
  auto start_pair3 = std::chrono::high_resolution_clock::now();
  rectifyFullResolutionImages(image_762_, image_761_, STEREO_EXTRINSICS_3);
  patches_map_left = splitImageTo4Patches(left_image_rectified_);
  patches_map_right = splitImageTo4Patches(right_image_rectified_);
  for (const auto &[name, roi] : patches_map_left) {
    cv::Mat left_image_patch = left_image_rectified_(roi);
    cv::Mat right_image_patch = right_image_rectified_(roi);
    assert(left_image_patch.size() == right_image_patch.size());
    assert(left_image_patch.cols == model_input_width_);
    assert(left_image_patch.rows == model_input_height_);

    processed_pair_760_759 =
        preprocessInputs(left_image_patch, right_image_patch);
    s2m2_output_buffers = runInference(processed_pair_760_759);

    int pixel_offset_x =
        (name == "top_left" || name == "bottom_left") ? 0 : roi.width;
    int pixel_offset_y =
        (name == "top_left" || name == "top_right") ? 0 : roi.height;
    cv::cuda::GpuMat &disp_gpu = s2m2_output_buffers[0];
    cv::Mat Qmatrix = rectifier_.getQMatrix();
    Qmatrix.convertTo(Qmatrix, CV_32FC1);
    cv::cuda::GpuMat points3d_homog_gpu =
        runReprojectionTo3D(disp_gpu, Qmatrix, pixel_offset_x, pixel_offset_y);

    cv::cuda::GpuMat transformed_points3d_gpu =
        runTransformationWC(points3d_homog_gpu, STEREO_EXTRINSICS_3);

    cv::cuda::GpuMat &occlusion_gpu = s2m2_output_buffers[1];
    cv::cuda::GpuMat confidence_gpu = s2m2_output_buffers[2];
    cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
    filterPoints3D(transformed_points3d_gpu, confidence_gpu, occlusion_gpu,
                   cv_stream);

    publishMessages(transformed_points3d_gpu);
  }
  auto end_pair3 = std::chrono::high_resolution_clock::now();
  auto dur_ms_pair3 =
      std::chrono::duration<double, std::milli>(end_pair3 - start_pair3)
          .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Total duration pair 3: %.3f ms",
              dur_ms_pair3);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Pair 3 duration: " << dur_ms_pair3 << "ms" << std::endl;
  }
}

void S2M2Node::loadCudaBuffers() {
  // TensorRT 10.13.3 uses tensor names instead of binding indices :
  // Get the names from the engine using int indices:
  const char *input_left_name = engine_->getIOTensorName(0);
  const char *input_right_name = engine_->getIOTensorName(1);
  const char *output_disp_name = engine_->getIOTensorName(2);
  const char *output_occ_name = engine_->getIOTensorName(3);
  const char *output_conf_name = engine_->getIOTensorName(4);

  // Use the helper func getSizeFromBinding to get the size of the input
  // tensors:
  size_t sizeInputLeft = getSizeFromBinding(engine_, input_left_name);
  size_t sizeInputRight = getSizeFromBinding(engine_, input_right_name);
  size_t sizeOutputDisp = getSizeFromBinding(engine_, output_disp_name);
  size_t sizeOutputOcc = getSizeFromBinding(engine_, output_occ_name);
  size_t sizeOutputConf = getSizeFromBinding(engine_, output_conf_name);
  // And 2
  // create the buffers -- cuda allocation happens on construction of the
  // buffer
  buffers_.emplace_back(std::move(CudaBuffer(sizeInputLeft)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeInputRight)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeOutputDisp)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeOutputOcc)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeOutputConf)));

  // Add a buffer for the points3D that will be produced with
  // reprojectImageTo3D:
  size_t sizePoints3DHomog =
      this->get_parameter("model_input_width").as_int() *
      this->get_parameter("model_input_height").as_int() * HOMOGENEOUS_DIMS;
  buffers_.emplace_back(std::move(CudaBuffer(sizePoints3DHomog)));
}

void S2M2Node::rectifyFullResolutionImages(cv::Mat &left_image,
                                           cv::Mat &right_image,
                                           const StereoExtrinsics &extrinsics) {
  //=====================RECTIFICATION===============================
  auto start_rectify = std::chrono::high_resolution_clock::now();
  rectifier_.calculateMaps(extrinsics);
#if DEBUG_MODE
  double baseline = rectifier_.getBaseline();
  RCLCPP_INFO(this->get_logger(), "----Baseline of rectified stereo: %f",
              baseline);
#endif
  auto start_rectify_left = std::chrono::high_resolution_clock::now();
  left_image_rectified_ = rectifier_.rectifyLeft(left_image);

  auto end_rectify_left = std::chrono::high_resolution_clock::now();
  auto dur_ms_rectify_left = std::chrono::duration<double, std::milli>(
                                 end_rectify_left - start_rectify_left)
                                 .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Rectification left duration: %.3f ms",
              dur_ms_rectify_left);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Rectification left duration: " << dur_ms_rectify_left
                   << "ms" << std::endl;
  }
  auto start_rectify_right = std::chrono::high_resolution_clock::now();
  right_image_rectified_ = rectifier_.rectifyRight(right_image);
  auto end_rectify_right = std::chrono::high_resolution_clock::now();
  auto dur_ms_right = std::chrono::duration<double, std::milli>(
                          end_rectify_right - start_rectify_right)
                          .count();
  auto dur_ms_total = std::chrono::duration<double, std::milli>(
                          end_rectify_right - start_rectify)
                          .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Rectification right duration: %.3f ms",
              dur_ms_right);
  RCLCPP_INFO(this->get_logger(), "Rectification total duration: %.3f ms",
              dur_ms_total);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Rectification right duration: " << dur_ms_right << "ms"
                   << std::endl;
    profiler_file_ << "Rectification total duration: " << dur_ms_total << "ms"
                   << std::endl;
  }
  if (this->get_parameter("save_rectified_images").as_bool()) {
    saveRectifiedImages(
        left_image_rectified_, right_image_rectified_,
        this->get_parameter("rectified_images_dirpath").as_string() +
            "rectified_left.png",
        this->get_parameter("rectified_images_dirpath").as_string() +
            "rectified_right.png");
  }
}

std::vector<cv::Mat> S2M2Node::preprocessInputs(cv::Mat &left_image,
                                                cv::Mat &right_image) {
  cv::Mat left_image_processed;
  cv::Mat right_image_processed;
  //=====================PADDING===================================
  auto start_pad = std::chrono::high_resolution_clock::now();
  // padding to multiples of 32 for the model :
  new_height_ =
      static_cast<int>(ceil(static_cast<float>(left_image.rows) / 32.0) * 32);
  new_width_ =
      static_cast<int>(ceil(static_cast<float>(left_image.cols) / 32.0) * 32);
  int pad_bottom = new_height_ - left_image.rows;
  int pad_right = new_width_ - left_image.cols;
  cv::copyMakeBorder(left_image, left_image_processed, 0, pad_bottom, 0,
                     pad_right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  cv::copyMakeBorder(right_image, right_image_processed, 0, pad_bottom, 0,
                     pad_right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  auto end_pad = std::chrono::high_resolution_clock::now();
  auto dur_ms_pad =
      std::chrono::duration<double, std::milli>(end_pad - start_pad).count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Padding duration: %.3f ms", dur_ms_pad);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Padding duration: " << dur_ms_pad << "ms" << std::endl;
  }
  // Create processed left image ROS2 msg befoe making it NCHW:
  cv_bridge::CvImage cv_image(left_image_msg_.header, "rgb8",
                              left_image_processed);
  cv_image.toImageMsg(left_image_msg_);
  auto start_nchw = std::chrono::high_resolution_clock::now();
  //=====================NCHW conversion===================================
  convertToNCHW(left_image_processed);
  convertToNCHW(right_image_processed);
  auto end_nchw = std::chrono::high_resolution_clock::now();
  auto dur_ms_nchw =
      std::chrono::duration<double, std::milli>(end_nchw - start_nchw).count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "NCHW conversion duration: %.3f ms",
              dur_ms_nchw);
#endif
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "NCHW conversion duration: " << dur_ms_nchw << "ms"
                   << std::endl;
  }
  return {left_image_processed, right_image_processed};
}

void S2M2Node::convertToNCHW(cv::Mat &image) {
  std::vector<cv::Mat> channels;
  cv::split(image, channels);
  cv::vconcat(channels, image);
}

TransformMatrix
S2M2Node::generateTransformWC(const StereoExtrinsics &extrinsics) {

  TransformMatrix T_WC;
  cv::Mat R_final_WC;
  cv::Mat t_final_WC;

  // We first get the extrinsics from pix4d of the unrectified left camera:
  cv::Mat R_CW;
  cv::Mat t_CW;
  extrinsics.R_left.convertTo(R_CW, CV_32FC1);
  extrinsics.t_left.convertTo(t_CW, CV_32FC1);
  cv::Mat R_WC_unrectified = R_CW.t();
  cv::Mat t_WC_unrectified = -R_WC_unrectified * t_CW;

  // We get also the rectified left camera extrinsics, and reverse them:
  cv::Mat R_left_rectified = rectifier_.getR1_rectified();
  R_left_rectified.convertTo(R_left_rectified, CV_32FC1);
  cv::Mat R_left_rectified_inv = R_left_rectified.t();

  // The final transform is the product of the two:
  R_final_WC = R_WC_unrectified * R_left_rectified_inv;
  t_final_WC = t_WC_unrectified; // rectifier does not translate

  // row major order of each float:
  // [R|t] 4x4 matrix in row major order
  // row 1:
  T_WC.data[0] = R_final_WC.at<float>(0, 0);
  T_WC.data[1] = R_final_WC.at<float>(0, 1);
  T_WC.data[2] = R_final_WC.at<float>(0, 2);
  T_WC.data[3] = t_final_WC.at<float>(0, 0);
  // row 2:
  T_WC.data[4] = R_final_WC.at<float>(1, 0);
  T_WC.data[5] = R_final_WC.at<float>(1, 1);
  T_WC.data[6] = R_final_WC.at<float>(1, 2);
  T_WC.data[7] = t_final_WC.at<float>(1, 0);
  // row 3:
  T_WC.data[8] = R_final_WC.at<float>(2, 0);
  T_WC.data[9] = R_final_WC.at<float>(2, 1);
  T_WC.data[10] = R_final_WC.at<float>(2, 2);
  T_WC.data[11] = t_final_WC.at<float>(2, 0);
  // row 4(homogeneous):
  T_WC.data[12] = 0.0f;
  T_WC.data[13] = 0.0f;
  T_WC.data[14] = 0.0f;
  T_WC.data[15] = 1.0f;
  // print the R,t matrices individually:
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "R_WC final: \n");
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      RCLCPP_INFO(this->get_logger(), "%f ", R_final_WC.at<float>(i, j));
    }
  }
  RCLCPP_INFO(this->get_logger(), "t_WC final: \n");
  for (int i = 0; i < 3; i++) {
    RCLCPP_INFO(this->get_logger(), "%f ", t_final_WC.at<float>(i, 0));
  }
#endif
  return T_WC;
}

std::vector<cv::cuda::GpuMat>
S2M2Node::runInference(std::vector<cv::Mat> &processed_pair) {
  // We verify that the images have the same size as the allocated cuda
  // buffers:
  cv::Mat left_image = processed_pair[0];
  cv::Mat right_image = processed_pair[1];
  size_t left_img_bytes = left_image.total() * left_image.elemSize();
  size_t right_img_bytes = right_image.total() * right_image.elemSize();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Verifying image sizes...");
  if (left_img_bytes != buffers_.at(0).size()) {
    RCLCPP_ERROR(this->get_logger(), "Left size : %ld", left_img_bytes);
    RCLCPP_ERROR(this->get_logger(), "Buffer size : %ld",
                 buffers_.at(0).size());
    RCLCPP_ERROR(this->get_logger(), "Left image shape : %d x %d",
                 left_image.cols, left_image.rows);
    throw std::runtime_error("left image size mismatch");
  }
  if (right_img_bytes != buffers_.at(1).size()) {
    RCLCPP_ERROR(this->get_logger(), "Right size : %ld", right_img_bytes);
    RCLCPP_ERROR(this->get_logger(), "Buffer size : %ld",
                 buffers_.at(1).size());
    throw std::runtime_error("right image size mismatch");
  }
  RCLCPP_INFO(this->get_logger(), "---Size checks passed---");
  RCLCPP_INFO(this->get_logger(), "Copying images to cuda buffers...");
#endif
  // Copy the images to the cuda buffers:
  cudaMemcpy(buffers_.at(0).data(), left_image.data, left_img_bytes,
             cudaMemcpyHostToDevice);
  cudaMemcpy(buffers_.at(1).data(), right_image.data, right_img_bytes,
             cudaMemcpyHostToDevice);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Setting tensor addresses...");
#endif
  // Run inference, by passing the tensors to the model :
  context_->setTensorAddress("input_left", buffers_.at(0).data());
  context_->setTensorAddress("input_right", buffers_.at(1).data());
  context_->setTensorAddress("output_disp", buffers_.at(2).data());
  context_->setTensorAddress("output_occ", buffers_.at(3).data());
  context_->setTensorAddress("output_conf", buffers_.at(4).data());
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Running inference...");
#endif
  // Start inference timer:
  cudaEventRecord(start_inference_, stream_);
  context_->enqueueV3(stream_);
  cudaEventRecord(end_inference_, stream_);
  cv::cuda::GpuMat disp_gpu(new_height_, new_width_, CV_32FC1,
                            buffers_.at(2).data());
  cv::cuda::GpuMat occlusion_out_buffer_gpu(new_height_, new_width_, CV_32FC1,
                                            buffers_.at(3).data());
  cv::cuda::GpuMat confidence_out_buffer_gpu(new_height_, new_width_, CV_32FC1,
                                             buffers_.at(4).data());

  cudaStreamSynchronize(stream_);
  float elapsed_inference_ms;
  cudaEventElapsedTime(&elapsed_inference_ms, start_inference_, end_inference_);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Inference in GPU duration: %.3f ms",
              elapsed_inference_ms);
#endif
  profiler_file_ << "Inference in GPU duration: " << elapsed_inference_ms
                 << "ms" << std::endl;
  return {disp_gpu, occlusion_out_buffer_gpu, confidence_out_buffer_gpu};
}

cv::cuda::GpuMat S2M2Node::runReprojectionTo3D(cv::cuda::GpuMat &disp,
                                               cv::Mat &Qmatrix,
                                               const int patch_offset_x,
                                               const int patch_offset_y) {
  cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
  cudaEventRecord(start_reproject_, stream_);
  cv::cuda::GpuMat points3d_homog_gpu(new_height_, new_width_, CV_32FC4);
  launchReprojectionCustomKernel(disp, points3d_homog_gpu, Qmatrix.ptr<float>(),
                                 patch_offset_x, patch_offset_y, stream_);
  cudaEventRecord(end_reproject_, stream_);
  cudaStreamSynchronize(stream_);
  float elapsed_reproject_ms;
  cudaEventElapsedTime(&elapsed_reproject_ms, start_reproject_, end_reproject_);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Reprojection in GPU duration: %.3f ms",
              elapsed_reproject_ms);
#endif
  profiler_file_ << "Reprojection in GPU duration: " << elapsed_reproject_ms
                 << "ms" << std::endl;
  return points3d_homog_gpu;
}

cv::cuda::GpuMat
S2M2Node::runTransformationWC(cv::cuda::GpuMat &points3d_homog_gpu,
                              const StereoExtrinsics &extrinsics) {
  TransformMatrix T_WC = generateTransformWC(extrinsics);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "The T matrix is: \n");
  for (int i = 0; i < 16; i++) {
    RCLCPP_INFO(this->get_logger(), "%f ", T_WC.data[i]);
  }
#endif
  cudaEventRecord(start_transform_kernel_, stream_);
  cv::cuda::GpuMat points3d_transformed_homog_gpu(new_height_, new_width_,
                                                  CV_32FC4);
  launchTransformKernel(T_WC, points3d_homog_gpu,
                        points3d_transformed_homog_gpu, new_height_, new_width_,
                        stream_);
  cudaEventRecord(end_transform_kernel_, stream_);
  cudaStreamSynchronize(stream_);
  float elapsed_transform_kernel_ms;
  cudaEventElapsedTime(&elapsed_transform_kernel_ms, start_transform_kernel_,
                       end_transform_kernel_);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(),
              "Coordinate transform CUDA kernel duration: %.3f ms",
              elapsed_transform_kernel_ms);
#endif
  profiler_file_ << "Coordinate transform CUDA kernel duration: "
                 << elapsed_transform_kernel_ms << "ms" << std::endl;
  return points3d_transformed_homog_gpu;
}

void S2M2Node::filterPoints3D(cv::cuda::GpuMat &points3d,
                              cv::cuda::GpuMat &confidence,
                              cv::cuda::GpuMat &occlusion,
                              cv::cuda::Stream &stream) {
  cudaEventRecord(start_filter_, stream_);
  // Filter points3d based on confidence and occlusion:
  cv::cuda::GpuMat occ_mask, conf_mask, valid_mask;
  cv::cuda::GpuMat invalid_mask;

  // create the masks:
  cv::cuda::compare(confidence, cv::Scalar(confidence_threshold_), conf_mask,
                    cv::CMP_GT, stream);
  cv::cuda::compare(occlusion, cv::Scalar(occlusion_threshold_), occ_mask,
                    cv::CMP_GT, stream);

  // combine masks:
  cv::cuda::bitwise_and(conf_mask, occ_mask, valid_mask, cv::noArray(), stream);
  cv::cuda::bitwise_not(valid_mask, invalid_mask, cv::noArray(), stream);

  // filter points3d:
  points3d.setTo(0, invalid_mask, stream); // Set invalid points to 0(!)
  cudaEventRecord(end_filter_, stream_);
  cudaStreamSynchronize(stream_);
  float elapsed_filter_ms;
  cudaEventElapsedTime(&elapsed_filter_ms, start_filter_, end_filter_);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Filtering points3D in GPU duration: %.3f ms",
              elapsed_filter_ms);
#endif
  profiler_file_ << "Filtering points3D in GPU duration: " << elapsed_filter_ms
                 << "ms" << std::endl;
}

void S2M2Node::publishMessages(
    const cv::cuda::GpuMat &points3d_transformed_homog_gpu) {
  cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
  auto start_download_and_publish = std::chrono::high_resolution_clock::now();
  //  Download the 3D points from GPU to CPU :
  cv::Mat points3d_homog;
  points3d_transformed_homog_gpu.download(points3d_homog, cv_stream);
  cudaStreamSynchronize(stream_);
  // convert points3d to ROS message:
  sensor_msgs::msg::Image points3d_msg;
  points3d_msg.header.frame_id = "left_camera_optical_frame";
  cv_bridge::CvImage cv_image_points3d(points3d_msg.header, "32FC4",
                                       points3d_homog);
  cv_image_points3d.toImageMsg(points3d_msg);
  // Set all timestamps to now :
  auto time_now = this->now();
  left_image_msg_.header.stamp = time_now;
  points3d_msg.header.stamp = time_now;
  // Publish all messages:
  left_processed_pub_->publish(left_image_msg_);
  points3d_pub_->publish(points3d_msg);
  auto end_download_and_publish = std::chrono::high_resolution_clock::now();
  auto dur_ms_download_and_publish =
      std::chrono::duration<double, std::milli>(end_download_and_publish -
                                                start_download_and_publish)
          .count();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Download and publish duration: %.3f ms",
              dur_ms_download_and_publish);
#endif
  profiler_file_ << "Download and publish duration: "
                 << dur_ms_download_and_publish << "ms" << std::endl;
}

std::map<std::string, cv::Rect>
S2M2Node::splitImageTo4Patches(cv::Mat &original_img) {
  int mid_x = original_img.cols / 2;
  int mid_y = original_img.rows / 2;
  int width = original_img.cols;
  int height = original_img.rows;
  std::map<std::string, cv::Rect> patches_map;
  patches_map["top_left"] = cv::Rect(0, 0, mid_x, mid_y);
  patches_map["top_right"] = cv::Rect(mid_x, 0, width - mid_x, mid_y);
  patches_map["bottom_left"] = cv::Rect(0, mid_y, mid_x, height - mid_y);
  patches_map["bottom_right"] =
      cv::Rect(mid_x, mid_y, width - mid_x, height - mid_y);
  return patches_map;
}

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<S2M2Node>();
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
}
