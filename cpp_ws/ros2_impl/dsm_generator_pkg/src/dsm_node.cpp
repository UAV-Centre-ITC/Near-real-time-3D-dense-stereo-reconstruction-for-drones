#include "dsm_generator_pkg/dsm_node.hpp"
#include <ios>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <rclcpp/logging.hpp>
#include <sensor_msgs/image_encodings.hpp>

DSMNode::DSMNode() : Node("dsm_node") {
  // params:
  this->declare_parameter("depthmaps_per_dsm", 3);
  this->declare_parameter("points3d_topic_name", "/stereo/points3d");
  this->declare_parameter("save_dsm", false);
  this->declare_parameter("dsm_step_X", 0.05);
  this->declare_parameter("dsm_step_Y", 0.05);
  depthmaps_per_dsm_ = this->get_parameter("depthmaps_per_dsm").as_int();
  depthmaps_counter_ = 0;
  save_dsm_ = this->get_parameter("save_dsm").as_bool();
  depthmap_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
      this->get_parameter("points3d_topic_name").as_string(), rclcpp::QoS(1),
      std::bind(&DSMNode::depthmapCallback, this, std::placeholders::_1));
  depthmaps_buffer_.reserve(depthmaps_per_dsm_);
  dsm_edges_ = cv::Mat::zeros(2, 2, CV_32FC2);
  // RCLCPP_INFO(this->get_logger(), "DSM node initialized");
}

void DSMNode::depthmapCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg) {
  if (++depthmaps_counter_ > depthmaps_per_dsm_) {
    return; // we are done, ignore following messages.
  }
  depthmaps_buffer_.push_back(points3d_msg);
  if (depthmaps_buffer_.size() == static_cast<size_t>(depthmaps_per_dsm_)) {
    auto start_dsm = std::chrono::high_resolution_clock::now();
    findDSMEdges();
    generateDSM();
    auto end_dsm = std::chrono::high_resolution_clock::now();
    auto dur_ms_dsm =
        std::chrono::duration<double, std::milli>(end_dsm - start_dsm).count();
    RCLCPP_INFO(this->get_logger(),
                "--DSM generation complete-- (execution time: %.3f ms)",
                dur_ms_dsm);
    if (save_dsm_) {
      saveDsmToPng("dsm.png");
      saveDsmToTif();
    }
  }
}

void DSMNode::findDSMEdges() {
  float min_x = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::lowest();

  for (size_t N = 0; N < depthmaps_buffer_.size(); N++) {
    const cv::Mat points3d = cvMatFromImageMsg(depthmaps_buffer_[N]);

    // dimensions of the depthmap:
    int depthmap_width = points3d.cols;
    int depthmap_height = points3d.rows;

    int stride = 4; // make the search faster at the cost of quality
    for (int i = 0; i < depthmap_height - stride; i += stride) {
      for (int j = 0; j < depthmap_width - stride; j += stride) {
        const cv::Vec4f &pt = points3d.at<cv::Vec4f>(i, j);
        min_x = std::min(min_x, pt[0]);
        max_x = std::max(max_x, pt[0]);
        min_y = std::min(min_y, pt[1]);
        max_y = std::max(max_y, pt[1]);
      }
    }
  }

  dsm_edges_.at<cv::Vec2f>(0, 0) = cv::Vec2f(min_x, min_y);
  dsm_edges_.at<cv::Vec2f>(0, 1) = cv::Vec2f(max_x, min_y);
  dsm_edges_.at<cv::Vec2f>(1, 0) = cv::Vec2f(min_x, max_y);
  dsm_edges_.at<cv::Vec2f>(1, 1) = cv::Vec2f(max_x, max_y);

  // // debug print :
  // RCLCPP_INFO(this->get_logger(), "DSM edges:");
  // for (int i = 0; i < dsm_edges_.rows; i++) {
  //   for (int j = 0; j < dsm_edges_.cols; j++) {
  //     RCLCPP_INFO(this->get_logger(), "%f %f}",
  //                 dsm_edges_.at<cv::Vec2f>(i, j)[0],
  //                 dsm_edges_.at<cv::Vec2f>(i, j)[1]);
  //   }
  // }
}

void DSMNode::generateDSM() {
  float max_x = dsm_edges_.at<cv::Vec2f>(1, 1)[0];
  float max_y = dsm_edges_.at<cv::Vec2f>(1, 1)[1];

  float min_x = dsm_edges_.at<cv::Vec2f>(0, 0)[0];
  float min_y = dsm_edges_.at<cv::Vec2f>(0, 0)[1];

  min_X_ = min_x; // used only for the .tfw file
  max_Y_ = max_y; // used only for the .tfw file

  step_X_ = this->get_parameter("dsm_step_X").as_double();
  step_Y_ = this->get_parameter("dsm_step_Y").as_double();
  dsm_width_ = (max_x - min_x) / step_X_;
  dsm_height_ = (max_y - min_y) / step_Y_;
  dsm_ = cv::Mat::zeros(dsm_height_, dsm_width_, CV_32FC1);

  // RCLCPP_INFO(this->get_logger(),
  //             "Starting the DSM generation with dimensions : %d x %d",
  //             dsm_width_, dsm_height_);
  for (size_t N = 0; N < depthmaps_buffer_.size(); N++) {
    const cv::Mat points3d = cvMatFromImageMsg(depthmaps_buffer_[N]);
    // dimensions of the depthmap:
    int depthmap_width = points3d.cols;
    int depthmap_height = points3d.rows;

    for (int row = 0; row < depthmap_height; row++) {
      for (int col = 0; col < depthmap_width; col++) {
        const cv::Vec4f &pt = points3d.at<cv::Vec4f>(row, col);
        // quantize into the DSM :
        int dsm_x = static_cast<int>((pt[0] - min_x) / step_X_);
        int dsm_y = static_cast<int>((pt[1] - min_y) / step_Y_);

        // check validity of that point :
        if (dsm_x > dsm_width_ - 1 || dsm_x < 0 || dsm_y > dsm_height_ - 1 ||
            dsm_y < 0) {
          continue;
        }
        // check if occupied in the DSM => choose max Z (closest to camera)
        if (dsm_.at<float>(dsm_y, dsm_x) < pt[2]) {
          dsm_.at<float>(dsm_y, dsm_x) = pt[2]; // depth value Z
        }
      }
    }
  }
  // RCLCPP_INFO(this->get_logger(), "--DSM generation complete--");
}

const cv::Mat
DSMNode::cvMatFromImageMsg(const sensor_msgs::msg::Image::ConstSharedPtr &msg) {
  cv_bridge::CvImageConstPtr ptr;
  ptr = cv_bridge::toCvShare(msg, "32FC4");
  return ptr->image;
}

void DSMNode::saveDsmToPng(const std::string &filepath) {
  // normalize the Z-values inside the DSM into unsigned int 0-255:
  cv::Mat dsm_normalized;
  cv::Mat dsm_normalized_colormapped;
  cv::normalize(dsm_, dsm_normalized, 0, 255, cv::NORM_MINMAX, CV_8UC1);
  cv::applyColorMap(dsm_normalized, dsm_normalized_colormapped,
                    cv::COLORMAP_JET);
  cv::imwrite(filepath, dsm_normalized_colormapped);
}

void DSMNode::saveDsmToTif() {
  cv::imwrite("dsm.tif", dsm_);
  // and the world file containing the metadata :
  std::ofstream tfw("dsm.tfw");
  tfw << std::fixed << std::setprecision(6);
  tfw << step_X_ << "\n";
  tfw << 0.0 << "\n";
  tfw << 0.0 << "\n";
  tfw << -step_Y_ << "\n";
  tfw << min_X_ << "\n";
  tfw << max_Y_ << "\n";

  tfw.close();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  rclcpp::init(argc, argv);
  std::shared_ptr<DSMNode> dsm_node = std::make_shared<DSMNode>();
  rclcpp::spin(dsm_node);
  rclcpp::shutdown();
  return 0;
}
