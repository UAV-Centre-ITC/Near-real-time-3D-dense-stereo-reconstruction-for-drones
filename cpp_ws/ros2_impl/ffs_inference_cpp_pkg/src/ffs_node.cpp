#include "ffs_inference_cpp_pkg/ffs_node.hpp"
#include "ffs_inference_cpp_pkg/cuda_kernels.h"
#include <chrono>
#include <cstdlib>
#include <dlfcn.h>
#include <functional>
#include <memory>
#include <mutex>
#include <opencv2/core/base.hpp>
#include <opencv2/core/types.hpp>
#include <ratio>
#include <rclcpp/duration.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/logging.hpp>

#define DEBUG_MODE true
#define HOMOGENEOUS_DIMS 4
#define NON_HOMOGENEOUS_DIMS 3

FFSNode::FFSNode() : Node("ffs_node"), rectifier_{STEREO_INTRINSICS, true} {

#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "FFSNode constructor");
#endif
  auto start_total = this->get_clock()->now();
  // Params :
  this->declare_parameter("engine_filepath", "");
  this->declare_parameter("plugin_filepath", "");
  this->declare_parameter("left_image_path", "");
  this->declare_parameter("right_image_path", "");
  this->declare_parameter("image_topic", "");
  this->declare_parameter("transport_hints", "raw");
  this->declare_parameter("absolute_pose_topic", "");
  this->declare_parameter("scale_factor", 0.5);
  this->declare_parameter("model_input_height", 1728);
  this->declare_parameter("model_input_width", 2304);
  this->declare_parameter("save_disparity", false);
  this->declare_parameter("save_rectified_images", false);
  this->declare_parameter("rectified_images_dirpath", "");
  this->declare_parameter("visualize_disparity", false);
  this->declare_parameter("time_profiling_dirpath", "");
  this->declare_parameter("min_keyframe_movement", 0.8);
  this->declare_parameter("max_keyframe_angular", 0.2);
  this->declare_parameter("max_keyframe_Z", 0.05);

  // Get params :
  engine_path_ = this->get_parameter("engine_filepath").as_string();
  model_input_height_ = this->get_parameter("model_input_height").as_int();
  model_input_width_ = this->get_parameter("model_input_width").as_int();
  profiler_dirpath_ = this->get_parameter("time_profiling_dirpath").as_string();
  std::string image_topic = this->get_parameter("image_topic").as_string();
  std::string absolute_pose_topic =
      this->get_parameter("absolute_pose_topic").as_string();
  std::string transport_hints =
      this->get_parameter("transport_hints").as_string();
  min_keyframe_movement_ =
      this->get_parameter("min_keyframe_movement").as_double();
  max_keyframe_angular_ =
      this->get_parameter("max_keyframe_angular").as_double();
  max_keyframe_Z_ = this->get_parameter("max_keyframe_Z").as_double();

  profiler_file_ =
      std::ofstream(profiler_dirpath_ + "profiler_ffsnode.txt", std::ios::out);

  std::string plugin_path = this->get_parameter("plugin_filepath").as_string();
  const char *plugin_path_c = plugin_path.c_str();

  void *handle = dlopen(plugin_path_c, RTLD_NOW | RTLD_GLOBAL);
  if (!handle) {
    RCLCPP_ERROR(this->get_logger(), "Failed to load FFSGWCVolume plugin: %s",
                 dlerror());
  } else {
    RCLCPP_INFO(this->get_logger(), "Successfully loaded FFSGWCVolume plugin!");
  }

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

  // preallocate for output disparity float buffer and points3D Mat:
  disparity_out_buffer_.resize(model_input_height_ * model_input_width_);
  points3d_homog_ =
      cv::Mat::zeros(model_input_height_, model_input_width_, CV_32FC4);

  // cv cuda matrix allocation :
  points3d_homog_gpu_.create(model_input_height_, model_input_width_, CV_32FC4);
  points3d_transformed_homog_gpu_.create(model_input_height_,
                                         model_input_width_, CV_32FC4);
  // Publishers :
  points3d_pub_ =
      this->create_publisher<sensor_msgs::msg::Image>("stereo/points3d", 10);
  left_processed_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
      "stereo/left/processed", 10);
  // Synchronizer and subscribers :
  rclcpp::QoS reliable_qos(50);
  reliable_qos.reliable();
  uint32_t queue_size = 50;
  synchronizer_ =
      std::make_shared<message_filters::Synchronizer<ApproximateTimeImagePose>>(
          ApproximateTimeImagePose(queue_size));
  synchronizer_->registerCallback(
      std::bind(&FFSNode::syncCallback, this, std::placeholders::_1,
                std::placeholders::_2));
  synchronizer_->getPolicy()->setMaxIntervalDuration(
      rclcpp::Duration(0, 100000000)); // 100 ms
  image_sub_.subscribe(this, image_topic, transport_hints,
                       reliable_qos.get_rmw_qos_profile());
  absolute_pose_sub_.subscribe(this, absolute_pose_topic,
                               reliable_qos.get_rmw_qos_profile());
  synchronizer_->connectInput(image_sub_, absolute_pose_sub_);
  // prealocate for in/out cuda buffers
  buffers_.reserve(3); // 5 buffers for input left and right, disp
  loadCudaBuffers();
  // start the processing thread
  worker_thread_ = std::thread(&FFSNode::processingThread, this);
}

FFSNode::~FFSNode() {
  // cleanup::
  delete context_;
  delete engine_;
  delete runtime_;
  cudaEventDestroy(start_inference_);
  cudaEventDestroy(end_inference_);
  cudaEventDestroy(start_transform_kernel_);
  cudaEventDestroy(end_transform_kernel_);
  cudaEventDestroy(start_reproject_);
  cudaEventDestroy(end_reproject_);
  keep_running_ = false;
  condition_var_.notify_one();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

void FFSNode::syncCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr &image_msg,
    const geometry_msgs::msg::PoseStamped::ConstSharedPtr &absolute_pose_msg) {
  std::unique_lock<std::mutex> lock(mtx_);

  current_keyframe_image_msg_ = image_msg;
  current_keyframe_absolute_pose_msg_ = absolute_pose_msg;
  new_data_ = true;
  lock.unlock();
  condition_var_.notify_one(); // wake up the processing thread
}

void FFSNode::loadCudaBuffers() {
  // TensorRT 10.13.3 uses tensor names instead of binding indices :
  // Get the names from the engine using int indices:
  const char *input_left_name = engine_->getIOTensorName(0);
  const char *input_right_name = engine_->getIOTensorName(1);
  const char *output_disp_name = engine_->getIOTensorName(2);

  // Use the helper func getSizeFromBinding to get the size of the input
  // tensors:
  size_t sizeInputLeft = getSizeFromBinding(engine_, input_left_name);
  size_t sizeInputRight = getSizeFromBinding(engine_, input_right_name);
  size_t sizeOutputDisp = getSizeFromBinding(engine_, output_disp_name);
  // And 2
  // create the buffers -- cuda allocation happens on construction of the
  // buffer
  buffers_.emplace_back(std::move(CudaBuffer(sizeInputLeft)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeInputRight)));
  buffers_.emplace_back(std::move(CudaBuffer(sizeOutputDisp)));
}

std::vector<cv::Mat> FFSNode::preprocessInputs(cv::Mat &left_image,
                                               cv::Mat &right_image) {
  cv::Mat left_processed;
  cv::Mat right_processed;

  auto start_pad = std::chrono::high_resolution_clock::now();
  // padding to multiples of 32 for the model :
  new_height_ =
      static_cast<int>(ceil(static_cast<float>(left_image.rows) / 32.0) * 32);
  new_width_ =
      static_cast<int>(ceil(static_cast<float>(left_image.cols) / 32.0) * 32);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "New height: %d", new_height_);
  RCLCPP_INFO(this->get_logger(), "New width: %d", new_width_);
#endif
  assert(new_height_ == model_input_height_);
  assert(new_width_ == model_input_width_);
  int pad_bottom = new_height_ - left_image.rows;
  int pad_right = new_width_ - left_image.cols;
  cv::copyMakeBorder(left_image, left_processed, 0, pad_bottom, 0, pad_right,
                     cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  cv::copyMakeBorder(right_image, right_processed, 0, pad_bottom, 0, pad_right,
                     cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
#if DEBUG_MODE
  auto end_pad = std::chrono::high_resolution_clock::now();
  auto dur_ms_pad =
      std::chrono::duration<double, std::milli>(end_pad - start_pad).count();
  RCLCPP_INFO(this->get_logger(), "Padding duration: %.3f ms", dur_ms_pad);
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Padding duration: " << dur_ms_pad << "ms" << std::endl;
  }
#endif
  // create processed left image before making it NCHW:
  cv::Mat copy_left_processed = left_processed.clone();
  copy_left_processed.convertTo(copy_left_processed, CV_8UC3);
  cv_bridge::CvImage cv_image(left_image_msg_.header, "rgb8",
                              copy_left_processed);
  cv_image.toImageMsg(left_image_msg_);
  auto start_nchw = std::chrono::high_resolution_clock::now();
  // Change the CV HWC to NCHW for TensorRT and normalize to ImageNet mean :
  normalizeAndConvertToNCHW(left_processed);
  normalizeAndConvertToNCHW(right_processed);
#if DEBUG_MODE
  auto end_nchw = std::chrono::high_resolution_clock::now();
  auto dur_ms_nchw =
      std::chrono::duration<double, std::milli>(end_nchw - start_nchw).count();
  RCLCPP_INFO(this->get_logger(), "NCHW conversion duration: %.3f ms",
              dur_ms_nchw);
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "NCHW conversion duration: " << dur_ms_nchw << "ms"
                   << std::endl;
  }
#endif
  return {left_processed, right_processed};
}

void FFSNode::normalizeAndConvertToNCHW(cv::Mat &image) {
  std::vector<cv::Mat> channels(3);
  cv::split(image, channels);

  // Normalize with ImageNet mean and std:
  const float mean[] = {0.485f, 0.456f, 0.406f};
  const float std_inv[] = {1.f / 0.229f, 1.f / 0.224f, 1.f / 0.225f};
  for (int i = 0; i < 3; i++) {
    channels[i] = (channels[i] - mean[i]) * std_inv[i];
  }
  cv::vconcat(channels, image);
}
/*
 In ROS (REP 103 standard), for aerial vehicles:
- X = forward
- Y = right
- Z = up
The function here returns what the stereo setup is for the current image set.
If the drone did not move enough to consider the current image a keyframe, it
returns NO_KEYFRAME. If there was significant Z movement or significant angular
movement, we return RESET_KEYFRAME to indicate that we start anew to collect
keyframes.
*/
STEREO_SETUP_NOW FFSNode::evaluateKeyframe(
    geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg_copy) {

  Eigen::Isometry3d T_now;
  Eigen::Isometry3d T_previous;
  tf2::fromMsg(absolute_pose_msg_copy->pose, T_now);
  tf2::fromMsg(last_keyframe_absolute_pose_msg_->pose, T_previous);

  T_previous_to_now_ = T_now.inverse() * T_previous;

  // Angle check :
  Eigen::AngleAxisd angle_axis(T_previous_to_now_.linear());
  double angle = std::abs(angle_axis.angle());
  RCLCPP_INFO(this->get_logger(), "Relative angle: %f", angle);
  if (angle > max_keyframe_angular_) {
    RCLCPP_INFO(this->get_logger(), "Angle is too large. Resetting keyframes.");
    return STEREO_SETUP_NOW::RESET_KEYFRAME;
  }

  // Translation checks :
  Eigen::Vector3d t_previous_to_now = T_previous_to_now_.translation();
  double dx = t_previous_to_now.x();
  double dy = t_previous_to_now.y();
  double dz = t_previous_to_now.z();
  if (abs(dz) > max_keyframe_Z_) {
    RCLCPP_INFO(this->get_logger(),
                "Translation z is too large. Resetting keyframes.");
    return STEREO_SETUP_NOW::RESET_KEYFRAME;
  }
  if (abs(dx) > min_keyframe_movement_) {
#ifdef DEBUG_MODE
    RCLCPP_INFO(this->get_logger(),
                "~~~Valid Keyframes pair detected~~~\nMotion dX = %.3f , dY = "
                "%.3f , dZ = %.3f",
                dx, dy, dz);
#endif
    return STEREO_SETUP_NOW::HORIZONTAL_KEYFRAME;
  }

  // No keyframe detected, return NOKEYFRAME:
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(),
              "Translation vector is too small: %.3f , %.3f , %.3f", dx, dy,
              dz);
#endif
  return STEREO_SETUP_NOW::NO_KEYFRAME;
}

void FFSNode::processingThread() {
  // We use copies of the messages that the syncCallback sets, to follow the
  // safe producer/consumer pattern:
  geometry_msgs::msg::PoseStamped::ConstSharedPtr
      current_keyframe_absolute_pose_copy;
  sensor_msgs::msg::Image::ConstSharedPtr current_keyframe_image_copy;
  while (rclcpp::ok() && keep_running_) {
    // ------ Wait for new data ------
    std::unique_lock<std::mutex> lock(mtx_);
    condition_var_.wait(lock,
                        [this]() { return new_data_ || !this->keep_running_; });
    // spurious wakeup or notify was called, we continue:
    if (!keep_running_) {
      RCLCPP_WARN(get_logger(), "Processing thread exiting...");
      return;
    }
    // We should now capture the new data as copies, and then unlock the mutex
    // to allow new data to flow.
    current_keyframe_image_copy = current_keyframe_image_msg_;
    current_keyframe_absolute_pose_copy = current_keyframe_absolute_pose_msg_;
    new_data_ = false;
    lock.unlock();
    // ------ New data available, lock released, start evaluation of keyframe
    // ------
    if (is_first_frame_) {
      last_keyframe_image_msg_ = current_keyframe_image_copy;
      last_keyframe_absolute_pose_msg_ = current_keyframe_absolute_pose_copy;
      is_first_frame_ = false;
      continue; // We need to wait for the second image to start the algorithm
    }
    auto stereo_setup = evaluateKeyframe(current_keyframe_absolute_pose_copy);
    if (stereo_setup == STEREO_SETUP_NOW::RESET_KEYFRAME) {
      RCLCPP_WARN(this->get_logger(), "Resetting keyframes...");
      last_keyframe_absolute_pose_msg_ = current_keyframe_absolute_pose_copy;
      last_keyframe_image_msg_ = current_keyframe_image_copy;
      continue;
    }
    if (stereo_setup == STEREO_SETUP_NOW::NO_KEYFRAME) {
#if DEBUG_MODE
      RCLCPP_INFO(this->get_logger(),
                  "Received image is not a keyframe. Returning...");
#endif
      // Simply skip
      continue;
    }
    // Valid keyframe pair found.
    relative_Rt_previous_to_current_ =
        extractTransformComponents(T_previous_to_now_);
    cv::Mat left_image;
    cv::Mat right_image;
    setLeftRightImages(current_keyframe_image_copy, left_image, right_image);

    // run the whole stereo pipeline on the valid pair
    processValidPair(left_image, right_image,
                     current_keyframe_absolute_pose_copy);

    // Reset variables for next frame:
    last_keyframe_absolute_pose_msg_ = current_keyframe_absolute_pose_copy;
    last_keyframe_image_msg_ = current_keyframe_image_copy;
  }
}

void FFSNode::processValidPair(cv::Mat &left_image, cv::Mat &right_image,
                               geometry_msgs::msg::PoseStamped::ConstSharedPtr
                                   current_keyframe_absolute_pose_copy) {
  rectifyFullResolutionImages(left_image, right_image);
  std::map<std::string, cv::Rect> patches_map_left =
      splitImageTo4Patches(left_image_rectified_);
  std::vector<cv::Mat> processed_pair;
  cv::cuda::GpuMat disp_gpu;
  for (const auto &[patch, roi] : patches_map_left) {
    cv::Mat left_patch = left_image_rectified_(roi);
    cv::Mat right_patch = right_image_rectified_(roi);
    assert(left_patch.size() == right_patch.size());
    processed_pair = preprocessInputs(left_patch, right_patch);
    assert(processed_pair[0].rows ==
           model_input_height_ * 3); //*3 because of NCHW stacking
    assert(processed_pair[0].cols == model_input_width_);
    assert(processed_pair[1].rows ==
           model_input_height_ * 3); // *3 because of NCHW stacking
    assert(processed_pair[1].cols == model_input_width_);
    disp_gpu = runInference(processed_pair);
    // Reproject disparity to 3D points:
    int pixel_offset_x =
        (patch == "top_left" || patch == "bottom_left") ? 0 : roi.width;
    int pixel_offset_y =
        (patch == "top_left" || patch == "top_right") ? 0 : roi.height;
    cv::Mat Qmatrix = rectifier_.getQMatrix();
    Qmatrix.convertTo(Qmatrix, CV_32FC1);
    cv::cuda::GpuMat points3d_homog_gpu =
        runReprojectionTo3D(disp_gpu, Qmatrix, pixel_offset_x, pixel_offset_y);

    // Transform points3d to world coordinate system:
    TransformMatrix T_WC;
    if (current_frame_is_left_) {
      T_WC = generateTransformWC(current_keyframe_absolute_pose_copy);
    } else {
      T_WC = generateTransformWC(last_keyframe_absolute_pose_msg_);
    }
    cv::cuda::GpuMat transformed_points3d_gpu =
        runTransformationWC(points3d_homog_gpu, T_WC);

    // Publish the transformed points3d along with the (already made) left
    // image:
    publishMessages(transformed_points3d_gpu);
  }
}

std::map<std::string, cv::Rect>
FFSNode::splitImageTo4Patches(cv::Mat &original_img) {
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

TransformComponents
FFSNode::extractTransformComponents(Eigen::Isometry3d T_previous_to_now) {
  cv::Mat R_relative_cv{cv::Mat::zeros(3, 3, CV_64FC1)};
  cv::Mat t_relative_cv{cv::Mat::zeros(3, 1, CV_64FC1)};
  Eigen::Matrix3d R_relative = T_previous_to_now.linear();
  Eigen::Vector3d t_relative = T_previous_to_now.translation();
  cv::eigen2cv(R_relative, R_relative_cv);
  cv::eigen2cv(t_relative, t_relative_cv);
  current_frame_is_left_ = false; // assume current is right, so no transform
                                  // reversion is needed for OpenCV.
  if (t_relative_cv.at<double>(0) > 0) {
    // This means we need to reverse the transformation. Note that
    // cv::stereoRectify expects Tx < 0 for horizontal stereo(!), because it
    // expects R,t from Left -> Right, so left camera center is on NEGATIVE x.
    T_previous_to_now = T_previous_to_now.inverse();
    R_relative = T_previous_to_now.linear();
    t_relative = T_previous_to_now.translation();
    cv::eigen2cv(R_relative, R_relative_cv);
    cv::eigen2cv(t_relative, t_relative_cv);
    current_frame_is_left_ = true;
  }
  return {R_relative_cv, t_relative_cv};
}

cv::cuda::GpuMat
FFSNode::runTransformationWC(cv::cuda::GpuMat &points3d_homog_gpu,
                             TransformMatrix &T_WC) {
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

void FFSNode::setLeftRightImages(
    sensor_msgs::msg::Image::ConstSharedPtr current_keyframe_image,
    cv::Mat &left_image, cv::Mat &right_image) {

  if (current_frame_is_left_) {
    left_image = cv_bridge::toCvCopy(current_keyframe_image, "rgb8")->image;
    right_image = cv_bridge::toCvCopy(last_keyframe_image_msg_, "rgb8")->image;
  } else {
    left_image = cv_bridge::toCvCopy(last_keyframe_image_msg_, "rgb8")->image;
    right_image = cv_bridge::toCvCopy(current_keyframe_image, "rgb8")->image;
  }
  left_image.convertTo(left_image, CV_32FC3);
  right_image.convertTo(right_image, CV_32FC3);
}

TransformMatrix FFSNode::generateTransformWC(
    geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg_copy) {
  TransformMatrix T_WC;
  cv::Mat R_final_WC;
  cv::Mat t_final_WC;

  // Dissasemble to its R,t components :
  Eigen::Isometry3d T_absolute;
  tf2::fromMsg(absolute_pose_msg_copy->pose, T_absolute);
  cv::Mat R_absolute_cv_unrectified{cv::Mat::zeros(3, 3, CV_32FC1)};
  cv::Mat t_absolute_cv_unrectified{cv::Mat::zeros(3, 1, CV_32FC1)};
  Eigen::Matrix3d R_absolute = T_absolute.linear();
  Eigen::Vector3d t_absolute = T_absolute.translation();
  cv::eigen2cv(R_absolute, R_absolute_cv_unrectified);
  cv::eigen2cv(t_absolute, t_absolute_cv_unrectified);
  R_absolute_cv_unrectified.convertTo(R_absolute_cv_unrectified, CV_32FC1);
  t_absolute_cv_unrectified.convertTo(t_absolute_cv_unrectified, CV_32FC1);

  // We also need to un-do the rectification's rotation :
  cv::Mat R_left_rectification = rectifier_.getR1_rectified();
  R_left_rectification.convertTo(R_left_rectification, CV_32FC1);
  cv::Mat R_left_rectification_inv = R_left_rectification.t();

  // The final transform is the product:
  R_final_WC = R_absolute_cv_unrectified * R_left_rectification_inv;
  t_final_WC = t_absolute_cv_unrectified;

  // row major order of each float:
  // row 1 :
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
  T_WC.data[12] = 0.0;
  T_WC.data[13] = 0.0;
  T_WC.data[14] = 0.0;
  T_WC.data[15] = 1.0;
  return T_WC;
}

void FFSNode::rectifyFullResolutionImages(cv::Mat &left_image,
                                          cv::Mat &right_image) {
  //=====================RECTIFICATION===============================
  auto start_rectify = std::chrono::high_resolution_clock::now();
  rectifier_.calculateMaps(relative_Rt_previous_to_current_.R_rel,
                           relative_Rt_previous_to_current_.t_rel);
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

cv::cuda::GpuMat FFSNode::runReprojectionTo3D(cv::cuda::GpuMat &disp_gpu,
                                              cv::Mat &Qmatrix,
                                              const int patch_offset_x,
                                              const int patch_offset_y) {
  cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
  cudaEventRecord(start_reproject_, stream_);
  cv::cuda::GpuMat points3d_homog_gpu(new_height_, new_width_, CV_32FC4);
  launchReprojectionCustomKernel(disp_gpu, points3d_homog_gpu,
                                 Qmatrix.ptr<float>(), patch_offset_x,
                                 patch_offset_y, stream_);
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

cv::cuda::GpuMat FFSNode::runInference(std::vector<cv::Mat> &processed_pair) {
  cv::Mat left_image = processed_pair[0];
  cv::Mat right_image = processed_pair[1];
  // We verify that the images have the same size as the allocated cuda
  // buffers:
  auto start_inference = std::chrono::high_resolution_clock::now();
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Verifying image sizes...");
  size_t left_img_bytes = left_image.total() * left_image.elemSize();
  size_t right_img_bytes = right_image.total() * right_image.elemSize();
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
  context_->setTensorAddress("left", buffers_.at(0).data());
  context_->setTensorAddress("right", buffers_.at(1).data());
  context_->setTensorAddress("disp", buffers_.at(2).data());
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Running inference...");
#endif
  // Start inference timer:
  cudaEventRecord(start_inference_, stream_);
  context_->enqueueV3(stream_);
  cudaEventRecord(end_inference_, stream_);
  cv::cuda::GpuMat disp_gpu(model_input_height_, model_input_width_, CV_32FC1,
                            buffers_.at(2).data());
  cudaStreamSynchronize(stream_);
  float elapsed_inference_ms;
  cudaEventElapsedTime(&elapsed_inference_ms, start_inference_, end_inference_);
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Inference in GPU duration: %.3f ms",
              elapsed_inference_ms);
#endif
  profiler_file_ << "Inference in GPU duration: " << elapsed_inference_ms
                 << "ms" << std::endl;
  return disp_gpu;
}

void FFSNode::publishMessages(
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

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<FFSNode>();
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
}
