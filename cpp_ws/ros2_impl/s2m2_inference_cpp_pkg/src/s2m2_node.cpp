#include "s2m2_inference_cpp_pkg/s2m2_node.hpp"
#include "s2m2_inference_cpp_pkg/cuda_kernels.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <opencv2/core/base.hpp>
#include <opencv2/core/types.hpp>
#include <rclcpp/duration.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/logging.hpp>

#define HOMOGENEOUS_DIMS 4

namespace s2m2_inference_cpp_pkg {

S2M2Node::S2M2Node(const rclcpp::NodeOptions &options)
    : Node("s2m2_node", options), rectifier_{STEREO_INTRINSICS} {

  RCLCPP_DEBUG(this->get_logger(), "S2M2Node constructor");
  // Params :
  this->declare_parameter("engine_filepath", "");
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
  this->declare_parameter("save_original_images", false);
  this->declare_parameter("rectified_images_dirpath", "");
  this->declare_parameter("visualize_disparity", false);
  this->declare_parameter("time_profiling_dirpath", "");
  this->declare_parameter("min_keyframe_movement", 0.8);
  this->declare_parameter("max_keyframe_angular", 0.2);
  this->declare_parameter("max_keyframe_dZ", 0.05);
  this->declare_parameter("occlusion_threshold", 0.5);
  this->declare_parameter("confidence_threshold", 0.1);

  // Get params :
  engine_path_ = this->get_parameter("engine_filepath").as_string();
  model_input_height_ = this->get_parameter("model_input_height").as_int();
  model_input_width_ = this->get_parameter("model_input_width").as_int();
  profiler_dirpath_ = this->get_parameter("time_profiling_dirpath").as_string();
  std::string image_topic = this->get_parameter("image_topic").as_string();
  occlusion_threshold_ = this->get_parameter("occlusion_threshold").as_double();
  confidence_threshold_ =
      this->get_parameter("confidence_threshold").as_double();
  std::string absolute_pose_topic =
      this->get_parameter("absolute_pose_topic").as_string();
  std::string transport_hints =
      this->get_parameter("transport_hints").as_string();
  min_keyframe_movement_ =
      this->get_parameter("min_keyframe_movement").as_double();
  max_keyframe_angular_ =
      this->get_parameter("max_keyframe_angular").as_double();
  max_keyframe_dZ_ = this->get_parameter("max_keyframe_dZ").as_double();

  profiler_file_ =
      std::ofstream(profiler_dirpath_ + "profiler_s2m2node.txt", std::ios::out);

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

  // preallocate GPU images:
  points3d_homog_gpu_.create(model_input_height_, model_input_width_, CV_32FC4);
  points3d_transformed_homog_gpu_.create(model_input_height_,
                                         model_input_width_, CV_32FC4);
  // define reliable QoS :
  rclcpp::QoS reliable_qos(50);
  reliable_qos.reliable();

  // Publishers :
  points3d_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
      "stereo/points3d", reliable_qos);
  left_processed_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
      "stereo/left/processed", reliable_qos);
  // Synchronizer and subscribers :
  uint32_t queue_size = 50;
  synchronizer_ =
      std::make_shared<message_filters::Synchronizer<ApproximateTimeImagePose>>(
          ApproximateTimeImagePose(queue_size));
  synchronizer_->registerCallback(std::bind(&S2M2Node::syncCallback, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
  synchronizer_->getPolicy()->setMaxIntervalDuration(
      rclcpp::Duration(0, 100000000)); // 100 ms
  image_sub_.subscribe(this, image_topic, transport_hints,
                       reliable_qos.get_rmw_qos_profile());
  absolute_pose_sub_.subscribe(this, absolute_pose_topic,
                               reliable_qos.get_rmw_qos_profile());
  synchronizer_->connectInput(image_sub_, absolute_pose_sub_);
  // prealocate for in/out cuda buffers
  buffers_.reserve(
      5); // 5 buffers for input left and right, output disparity, occ, conf
  loadCudaBuffers();
  // start the processing thread
  worker_thread_ = std::thread(&S2M2Node::processingThread, this);
}

S2M2Node::~S2M2Node() {
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
  cudaEventDestroy(start_filter_);
  cudaEventDestroy(end_filter_);
  keep_running_ = false;
  condition_var_.notify_one();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

void S2M2Node::syncCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr &image_msg,
    const geometry_msgs::msg::PoseStamped::ConstSharedPtr &absolute_pose_msg) {
  std::unique_lock<std::mutex> lock(mtx_);
  current_keyframe_image_msg_ = image_msg;
  current_keyframe_absolute_pose_msg_ = absolute_pose_msg;
  new_data_ = true;
  lock.unlock();
  condition_var_.notify_one(); // wake up the processing thread
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
  size_t size_input_left = getSizeFromBinding(engine_, input_left_name);
  size_t size_input_right = getSizeFromBinding(engine_, input_right_name);
  size_t size_output_disp = getSizeFromBinding(engine_, output_disp_name);
  size_t size_output_occ = getSizeFromBinding(engine_, output_occ_name);
  size_t size_output_conf = getSizeFromBinding(engine_, output_conf_name);
  // Allocate CUDA buffers (allocation happens on construction):
  buffers_.emplace_back(size_input_left);
  buffers_.emplace_back(size_input_right);
  buffers_.emplace_back(size_output_disp);
  buffers_.emplace_back(size_output_occ);
  buffers_.emplace_back(size_output_conf);

  // Buffer for points3D produced by reprojectImageTo3D:
  size_t size_points3d_homog =
      this->get_parameter("model_input_width").as_int() *
      this->get_parameter("model_input_height").as_int() * HOMOGENEOUS_DIMS;
  buffers_.emplace_back(size_points3d_homog);
}

std::vector<cv::Mat> S2M2Node::preprocessInputs(cv::Mat &left_image,
                                                cv::Mat &right_image,
                                                PassTimings &pass_t) {
  auto start_pad = std::chrono::high_resolution_clock::now();

  // Bring images to the exact model input size:
  //   - larger  → resize down to model_input_* (full-resolution mode)
  //   - smaller → pad        to model_input_* (patch mode)
  cv::Mat left_fit, right_fit;
  if (left_image.rows > model_input_height_ ||
      left_image.cols > model_input_width_) {
    cv::resize(left_image, left_fit,
               cv::Size(model_input_width_, model_input_height_), 0, 0,
               cv::INTER_LINEAR);
    cv::resize(right_image, right_fit,
               cv::Size(model_input_width_, model_input_height_), 0, 0,
               cv::INTER_LINEAR);
  } else {
    left_fit = left_image;
    right_fit = right_image;
  }

  processed_img_height_ =
      static_cast<int>(ceil(static_cast<float>(left_fit.rows) / 32.0) * 32);
  processed_img_width_ =
      static_cast<int>(ceil(static_cast<float>(left_fit.cols) / 32.0) * 32);
  RCLCPP_DEBUG(this->get_logger(), "New height: %d", processed_img_height_);
  RCLCPP_DEBUG(this->get_logger(), "New width: %d", processed_img_width_);
  assert(processed_img_height_ == model_input_height_);
  assert(processed_img_width_ == model_input_width_);
  int pad_bottom = processed_img_height_ - left_fit.rows;
  int pad_right = processed_img_width_ - left_fit.cols;
  RCLCPP_DEBUG(this->get_logger(), "Padding bottom: %d", pad_bottom);
  RCLCPP_DEBUG(this->get_logger(), "Padding right: %d", pad_right);
  cv::Mat left_processed, right_processed;
  cv::copyMakeBorder(left_fit, left_processed, 0, pad_bottom, 0, pad_right,
                     cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  cv::copyMakeBorder(right_fit, right_processed, 0, pad_bottom, 0, pad_right,
                     cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  auto end_pad = std::chrono::high_resolution_clock::now();
  pass_t.padding_ms =
      std::chrono::duration<double, std::milli>(end_pad - start_pad).count();
  RCLCPP_DEBUG(this->get_logger(), "Preprocess fit duration: %.3f ms",
               pass_t.padding_ms);
  // Store a clone of the processed left image before NCHW conversion,
  // so it can be published later:
  left_pass_processed_ = left_processed.clone();
  auto start_nchw = std::chrono::high_resolution_clock::now();
  // Change the CV HWC to NCHW for TensorRT :
  convertToNCHW(left_processed);
  convertToNCHW(right_processed);
  auto end_nchw = std::chrono::high_resolution_clock::now();
  pass_t.nchw_ms =
      std::chrono::duration<double, std::milli>(end_nchw - start_nchw).count();
  RCLCPP_DEBUG(this->get_logger(), "NCHW conversion duration: %.3f ms",
               pass_t.nchw_ms);
  return {left_processed, right_processed};
}

void S2M2Node::convertToNCHW(cv::Mat &image) {
  std::vector<cv::Mat> channels;
  cv::split(image, channels);
  cv::vconcat(channels, image);
}
// In ROS (REP 103), for aerial vehicles: X = forward, Y = right, Z = up.
STEREO_SETUP_NOW S2M2Node::evaluateKeyframe(
    geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg_copy) {

  Eigen::Isometry3d T_now;
  Eigen::Isometry3d T_previous;
  tf2::fromMsg(absolute_pose_msg_copy->pose, T_now);
  tf2::fromMsg(last_keyframe_absolute_pose_msg_->pose, T_previous);

  T_previous_to_now_ = T_now.inverse() * T_previous;

  // Angle check :
  Eigen::AngleAxisd angle_axis(T_previous_to_now_.linear());
  double angle = std::abs(angle_axis.angle());
  RCLCPP_DEBUG(this->get_logger(), "Relative angle: %f", angle);
  if (angle > max_keyframe_angular_) {
    RCLCPP_INFO(this->get_logger(), "Angle is too large. Resetting keyframes.");
    return STEREO_SETUP_NOW::RESET_KEYFRAME;
  }

  // Translation checks :
  Eigen::Vector3d t_previous_to_now = T_previous_to_now_.translation();
  double dx = t_previous_to_now.x();
  double dy = t_previous_to_now.y();
  double dz = t_previous_to_now.z();
  if (abs(dz) > max_keyframe_dZ_) {
    RCLCPP_DEBUG(this->get_logger(),
                 "Translation z is too large. Resetting keyframes.");
    return STEREO_SETUP_NOW::RESET_KEYFRAME;
  }
  if (abs(dx) > min_keyframe_movement_) {
    RCLCPP_INFO(this->get_logger(),
                "~~~Valid Keyframes pair detected~~~\nMotion dX = %.3f , dY = "
                "%.3f , dZ = %.3f",
                dx, dy, dz);
    return STEREO_SETUP_NOW::VALID_PAIR;
  }

  // No keyframe detected, return NOKEYFRAME:
  RCLCPP_DEBUG(this->get_logger(),
               "Translation vector is too small: %.3f , %.3f , %.3f", dx, dy,
               dz);
  return STEREO_SETUP_NOW::NO_KEYFRAME;
}

void S2M2Node::processingThread() {
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
      RCLCPP_DEBUG(this->get_logger(),
                   "Received image is not a keyframe. Returning...");
      // Simply skip
      continue;
    }
    // Valid keyframe pair found.
    relative_Rt_previous_to_current_ =
        extractTransformComponents(T_previous_to_now_);
    cv::Mat left_image;
    cv::Mat right_image;
    setLeftRightImages(current_keyframe_image_copy, left_image, right_image);
    // Save the original images if specified, using their timestamps as file
    // name:
    if (this->get_parameter("save_original_images").as_bool()) {
      std::string left_name, right_name;
      if (current_frame_is_left_) {
        left_name =
            std::to_string(current_keyframe_image_copy->header.stamp.sec) +
            "_" +
            std::to_string(current_keyframe_image_copy->header.stamp.nanosec) +
            "_left.png";
        right_name =
            std::to_string(last_keyframe_image_msg_->header.stamp.sec) + "_" +
            std::to_string(last_keyframe_image_msg_->header.stamp.nanosec) +
            "_right.png";
      } else {
        left_name =
            std::to_string(last_keyframe_image_msg_->header.stamp.sec) + "_" +
            std::to_string(last_keyframe_image_msg_->header.stamp.nanosec) +
            "_left.png";
        right_name =
            std::to_string(current_keyframe_image_copy->header.stamp.sec) +
            "_" +
            std::to_string(current_keyframe_image_copy->header.stamp.nanosec) +
            "_right.png";
      }
      saveOriginalImages(left_image, right_image, left_name, right_name);
    }

    // Capture left-image timestamp for rectified image filenames:
    if (current_frame_is_left_) {
      left_rect_sec_ = current_keyframe_image_copy->header.stamp.sec;
      left_rect_nanosec_ = current_keyframe_image_copy->header.stamp.nanosec;
    } else {
      left_rect_sec_ = last_keyframe_image_msg_->header.stamp.sec;
      left_rect_nanosec_ = last_keyframe_image_msg_->header.stamp.nanosec;
    }

    // run the whole stereo pipeline on the valid pair
    processValidPair(left_image, right_image,
                     current_keyframe_absolute_pose_copy);

    // Reset variables for next frame:
    last_keyframe_absolute_pose_msg_ = current_keyframe_absolute_pose_copy;
    last_keyframe_image_msg_ = current_keyframe_image_copy;
  }
}

void S2M2Node::processValidPair(cv::Mat &left_image, cv::Mat &right_image,
                                geometry_msgs::msg::PoseStamped::ConstSharedPtr
                                    current_keyframe_absolute_pose_copy) {
  auto start_total_pair = std::chrono::high_resolution_clock::now();
  // Accumulate all timing samples for this pair and flush them to disk in a
  // single write at the very end (see bottom of this function).
  PairTimings timings;
  rectifyFullResolutionImages(left_image, right_image, timings);
  cv::Mat Qmatrix = rectifier_.getQMatrix();
  Qmatrix.convertTo(Qmatrix, CV_32FC1);

  // Calculate the Transformation Matrix that will be used
  TransformMatrix T_WC;
  if (current_frame_is_left_) {
    T_WC = generateTransformWC(current_keyframe_absolute_pose_copy);
  } else {
    T_WC = generateTransformWC(last_keyframe_absolute_pose_msg_);
  }

  // === Full-resolution pipeline ===
  PassTimings &pass_t = timings.pass;

  std::vector<cv::Mat> processed_pair =
      preprocessInputs(left_image_rectified_, right_image_rectified_, pass_t);
  assert(processed_pair[0].rows ==
         model_input_height_ * 3); //*3 because of NCHW stacking
  assert(processed_pair[0].cols == model_input_width_);
  assert(processed_pair[1].rows ==
         model_input_height_ * 3); // *3 because of NCHW stacking
  assert(processed_pair[1].cols == model_input_width_);
  std::vector<cv::cuda::GpuMat> s2m2_outputs =
      runInference(processed_pair, pass_t);

  // Reproject disparity to 3D points:
  cv::cuda::GpuMat &disp_gpu = s2m2_outputs[0];
  runReprojectionTo3D(disp_gpu, Qmatrix, 0, 0, pass_t);

  runTransformationWC(T_WC, pass_t);

  // Filter points3d based on confidence and occlusion:
  cv::cuda::GpuMat &occlusion_gpu = s2m2_outputs[1];
  cv::cuda::GpuMat &confidence_gpu = s2m2_outputs[2];
  cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
  filterPoints3D(confidence_gpu, occlusion_gpu, cv_stream, pass_t);

  publishMessages(pass_t);

  auto end_total_pair = std::chrono::high_resolution_clock::now();
  timings.pair_total_ms = std::chrono::duration<double, std::milli>(
                              end_total_pair - start_total_pair)
                              .count();
  RCLCPP_DEBUG(this->get_logger(), "Total valid pair duration: %.3f ms",
               timings.pair_total_ms);
  // Flush all accumulated timings for this pair with a single I/O call.
  if (profiler_writes_count_ < 50) {
    std::ostringstream oss;
    oss << "----- pair " << profiler_writes_count_ << " -----\n"
        << "Rectification left duration: " << timings.rectify_left_ms << "ms\n"
        << "Rectification right duration: " << timings.rectify_right_ms
        << "ms\n"
        << "Rectification total duration: " << timings.rectify_total_ms
        << "ms\n";
    const PassTimings &p = timings.pass;
    oss << "Padding duration: " << p.padding_ms << "ms\n"
        << "NCHW conversion duration: " << p.nchw_ms << "ms\n"
        << "Inference in GPU duration: " << p.inference_ms << "ms\n"
        << "Reprojection in GPU duration: " << p.reproject_ms << "ms\n"
        << "Coordinate transform CUDA kernel duration: " << p.transform_ms
        << "ms\n"
        << "Filter points3D in GPU duration: " << p.filter_ms << "ms\n"
        << "Download and publish duration: " << p.download_publish_ms << "ms\n"
        << "Total valid pair duration: " << timings.pair_total_ms << "ms\n";
    profiler_file_ << oss.str() << std::flush;
    ++profiler_writes_count_;
  }
}

TransformComponents
S2M2Node::extractTransformComponents(Eigen::Isometry3d T_previous_to_now) {
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
    // cv::stereoRectify expects Tx < 0 for horizontal stereo, because it
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

void S2M2Node::runTransformationWC(const TransformMatrix &T_WC,
                                   PassTimings &pass_t) {
  RCLCPP_DEBUG(this->get_logger(), "The T matrix is: \n");
  for (int i = 0; i < 16; i++) {
    RCLCPP_DEBUG(this->get_logger(), "%f ", T_WC.data[i]);
  }
  cudaEventRecord(start_transform_kernel_, stream_);
  launchTransformKernel(T_WC, points3d_homog_gpu_,
                        points3d_transformed_homog_gpu_, processed_img_height_,
                        processed_img_width_, stream_);
  cudaEventRecord(end_transform_kernel_, stream_);
  cudaEventSynchronize(end_transform_kernel_);
  float elapsed_transform_kernel_ms;
  cudaEventElapsedTime(&elapsed_transform_kernel_ms, start_transform_kernel_,
                       end_transform_kernel_);
  pass_t.transform_ms = static_cast<double>(elapsed_transform_kernel_ms);
  RCLCPP_DEBUG(this->get_logger(),
               "Coordinate transform CUDA kernel duration: %.3f ms",
               pass_t.transform_ms);
}

void S2M2Node::setLeftRightImages(
    sensor_msgs::msg::Image::ConstSharedPtr current_keyframe_image,
    cv::Mat &left_image, cv::Mat &right_image) {
  if (current_frame_is_left_) {
    left_image = cv_bridge::toCvCopy(current_keyframe_image, "rgb8")->image;
    right_image = cv_bridge::toCvCopy(last_keyframe_image_msg_, "rgb8")->image;
  } else {
    left_image = cv_bridge::toCvCopy(last_keyframe_image_msg_, "rgb8")->image;
    right_image = cv_bridge::toCvCopy(current_keyframe_image, "rgb8")->image;
  }
}

TransformMatrix S2M2Node::generateTransformWC(
    geometry_msgs::msg::PoseStamped::ConstSharedPtr absolute_pose_msg_copy) {
  TransformMatrix T_WC;
  cv::Mat R_final_WC;
  cv::Mat t_final_WC;

  // Decompose into R,t components:
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

  // Undo the rectification rotation:
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

void S2M2Node::rectifyFullResolutionImages(cv::Mat &left_image,
                                           cv::Mat &right_image,
                                           PairTimings &timings) {
  //=====================RECTIFICATION===============================
  auto start_rectify = std::chrono::high_resolution_clock::now();
  rectifier_.calculateMaps(relative_Rt_previous_to_current_.R_rel,
                           relative_Rt_previous_to_current_.t_rel);
  RCLCPP_DEBUG(this->get_logger(), "----Baseline of rectified stereo: %f",
               rectifier_.getBaseline());
  auto start_rectify_left = std::chrono::high_resolution_clock::now();
  left_image_rectified_ = rectifier_.rectifyLeft(left_image);

  auto end_rectify_left = std::chrono::high_resolution_clock::now();
  timings.rectify_left_ms = std::chrono::duration<double, std::milli>(
                                end_rectify_left - start_rectify_left)
                                .count();
  RCLCPP_DEBUG(this->get_logger(), "Rectification left duration: %.3f ms",
               timings.rectify_left_ms);
  auto start_rectify_right = std::chrono::high_resolution_clock::now();
  right_image_rectified_ = rectifier_.rectifyRight(right_image);
  auto end_rectify_right = std::chrono::high_resolution_clock::now();
  timings.rectify_right_ms = std::chrono::duration<double, std::milli>(
                                 end_rectify_right - start_rectify_right)
                                 .count();
  timings.rectify_total_ms = std::chrono::duration<double, std::milli>(
                                 end_rectify_right - start_rectify)
                                 .count();
  RCLCPP_DEBUG(this->get_logger(), "Rectification right duration: %.3f ms",
               timings.rectify_right_ms);
  RCLCPP_DEBUG(this->get_logger(), "Rectification total duration: %.3f ms",
               timings.rectify_total_ms);
  if (this->get_parameter("save_rectified_images").as_bool()) {
    saveRectifiedImages(
        left_image_rectified_, right_image_rectified_,
        this->get_parameter("rectified_images_dirpath").as_string() +
            "left_rectified_" + std::to_string(left_rect_sec_) + "_" +
            std::to_string(left_rect_nanosec_) + ".png",
        this->get_parameter("rectified_images_dirpath").as_string() +
            "right_rectified_" + std::to_string(left_rect_sec_) + "_" +
            std::to_string(left_rect_nanosec_) + ".png");
  }
}

void S2M2Node::filterPoints3D(cv::cuda::GpuMat &confidence,
                              cv::cuda::GpuMat &occlusion,
                              cv::cuda::Stream &stream, PassTimings &pass_t) {
  cv::cuda::GpuMat occ_mask, conf_mask, valid_mask, invalid_mask;
  cudaEventRecord(start_filter_, stream_);
  cv::cuda::compare(confidence, cv::Scalar(confidence_threshold_), conf_mask,
                    cv::CMP_GT, stream);
  cv::cuda::compare(occlusion, cv::Scalar(occlusion_threshold_), occ_mask,
                    cv::CMP_GT, stream);

  cv::cuda::bitwise_and(conf_mask, occ_mask, valid_mask, cv::noArray(), stream);
  cv::cuda::bitwise_not(valid_mask, invalid_mask, cv::noArray(), stream);
  points3d_transformed_homog_gpu_.setTo(0, invalid_mask, stream);

  cudaEventRecord(end_filter_, stream_);
  cudaEventSynchronize(end_filter_);
  float elapsed_filter_ms;
  cudaEventElapsedTime(&elapsed_filter_ms, start_filter_, end_filter_);
  pass_t.filter_ms = static_cast<double>(elapsed_filter_ms);
  RCLCPP_DEBUG(this->get_logger(), "Filter points3D in GPU duration: %.3f ms",
               pass_t.filter_ms);
}

void S2M2Node::runReprojectionTo3D(cv::cuda::GpuMat &disp_gpu,
                                   const cv::Mat &Qmatrix,
                                   const int patch_offset_x,
                                   const int patch_offset_y,
                                   PassTimings &pass_t) {
  cudaEventRecord(start_reproject_, stream_);
  launchReprojectionCustomKernel(disp_gpu, points3d_homog_gpu_,
                                 Qmatrix.ptr<float>(), patch_offset_x,
                                 patch_offset_y, stream_);
  cudaEventRecord(end_reproject_, stream_);
  cudaEventSynchronize(end_reproject_);
  float elapsed_reproject_ms;
  cudaEventElapsedTime(&elapsed_reproject_ms, start_reproject_, end_reproject_);
  pass_t.reproject_ms = static_cast<double>(elapsed_reproject_ms);
  RCLCPP_DEBUG(this->get_logger(), "Reprojection in GPU duration: %.3f ms",
               pass_t.reproject_ms);
}

std::vector<cv::cuda::GpuMat>
S2M2Node::runInference(std::vector<cv::Mat> &processed_pair,
                       PassTimings &pass_t) {
  cv::Mat left_image = processed_pair[0];
  cv::Mat right_image = processed_pair[1];
  // We verify that the images have the same size as the allocated cuda
  // buffers:
  auto start_inference = std::chrono::high_resolution_clock::now();
  size_t left_img_bytes = left_image.total() * left_image.elemSize();
  size_t right_img_bytes = right_image.total() * right_image.elemSize();
  RCLCPP_DEBUG(this->get_logger(), "Verifying image sizes...");
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
  RCLCPP_DEBUG(this->get_logger(), "---Size checks passed---");
  RCLCPP_DEBUG(this->get_logger(), "Copying images to cuda buffers...");
  // Copy the images to the cuda buffers:
  cudaMemcpyAsync(buffers_.at(0).data(), left_image.data, left_img_bytes,
                  cudaMemcpyHostToDevice, stream_);
  cudaMemcpyAsync(buffers_.at(1).data(), right_image.data, right_img_bytes,
                  cudaMemcpyHostToDevice, stream_);
  RCLCPP_DEBUG(this->get_logger(), "Setting tensor addresses...");
  // Run inference, by passing the tensors to the model :
  context_->setTensorAddress("input_left", buffers_.at(0).data());
  context_->setTensorAddress("input_right", buffers_.at(1).data());
  context_->setTensorAddress("output_disp", buffers_.at(2).data());
  context_->setTensorAddress("output_occ", buffers_.at(3).data());
  context_->setTensorAddress("output_conf", buffers_.at(4).data());
  RCLCPP_DEBUG(this->get_logger(), "Running inference...");
  // Start inference timer:
  cudaEventRecord(start_inference_, stream_);
  context_->enqueueV3(stream_);
  cudaEventRecord(end_inference_, stream_);
  cv::cuda::GpuMat disp_gpu(model_input_height_, model_input_width_, CV_32FC1,
                            buffers_.at(2).data());
  cv::cuda::GpuMat occ_gpu(model_input_height_, model_input_width_, CV_32FC1,
                           buffers_.at(3).data());
  cv::cuda::GpuMat conf_gpu(processed_img_height_, processed_img_width_,
                            CV_32FC1, buffers_.at(4).data());
  cudaEventSynchronize(end_inference_);
  float elapsed_inference_ms;
  cudaEventElapsedTime(&elapsed_inference_ms, start_inference_, end_inference_);
  pass_t.inference_ms = static_cast<double>(elapsed_inference_ms);
  RCLCPP_DEBUG(this->get_logger(), "Inference in GPU duration: %.3f ms",
               pass_t.inference_ms);
  return {disp_gpu, occ_gpu, conf_gpu};
}

void S2M2Node::publishMessages(PassTimings &pass_t) {
  cv::cuda::Stream cv_stream = cv::cuda::StreamAccessor::wrapStream(stream_);
  auto start_download_and_publish = std::chrono::high_resolution_clock::now();

  // Wait for all CUDA operations to finish
  cudaStreamSynchronize(stream_);

  // Allocate the unique_ptr for true zero-copy IPC
  auto points3d_msg = std::make_unique<sensor_msgs::msg::Image>();
  points3d_msg->header.frame_id = "left_camera_optical_frame";
  points3d_msg->header.stamp = this->now();
  points3d_msg->height = model_input_height_;
  points3d_msg->width = model_input_width_;
  points3d_msg->encoding = "32FC4";
  points3d_msg->is_bigendian = false;
  points3d_msg->step =
      model_input_width_ * 4 * sizeof(float); // 4 channels of 32-bit floats

  // Pre-allocate the exact size in the ROS message vector
  size_t points3d_size = points3d_msg->step * points3d_msg->height;
  points3d_msg->data.resize(points3d_size);

  // Wrap a cv::Mat AROUND the ROS message's memory buffer (No allocation!)
  cv::Mat points3d_mapped(model_input_height_, model_input_width_, CV_32FC4,
                          points3d_msg->data.data());

  // Download from GPU directly into the ROS message memory
  points3d_transformed_homog_gpu_.download(points3d_mapped, cv_stream);

  auto left_img_msg = std::make_unique<sensor_msgs::msg::Image>();
  left_img_msg->header.frame_id = "left_camera_optical_frame";
  left_img_msg->header.stamp = points3d_msg->header.stamp;
  left_img_msg->height = left_pass_processed_.rows;
  left_img_msg->width = left_pass_processed_.cols;
  left_img_msg->encoding = "rgb8";
  left_img_msg->is_bigendian = false;
  left_img_msg->step = left_pass_processed_.cols * 3;

  size_t left_size = left_img_msg->step * left_img_msg->height;
  left_img_msg->data.resize(left_size);

  // Wrap a cv::Mat around the ROS message and copy the CPU image directly into
  // it
  cv::Mat left_mapped(left_pass_processed_.rows, left_pass_processed_.cols,
                      CV_8UC3, left_img_msg->data.data());
  left_pass_processed_.copyTo(left_mapped);

  left_processed_pub_->publish(std::move(left_img_msg));
  points3d_pub_->publish(std::move(points3d_msg));

  auto end_download_and_publish = std::chrono::high_resolution_clock::now();
  pass_t.download_publish_ms =
      std::chrono::duration<double, std::milli>(end_download_and_publish -
                                                start_download_and_publish)
          .count();

  RCLCPP_DEBUG(this->get_logger(), "Download and publish duration: %.3f ms",
               pass_t.download_publish_ms);
}
} // namespace s2m2_inference_cpp_pkg

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(s2m2_inference_cpp_pkg::S2M2Node)

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<s2m2_inference_cpp_pkg::S2M2Node>();
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
}
