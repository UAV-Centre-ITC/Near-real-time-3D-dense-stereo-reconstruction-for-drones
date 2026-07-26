#include <string>
#define DEBUG_MODE false
#include "pointcloud_pkg/pointcloud_generator.hpp"

namespace pointcloud_pkg {

PointcloudGenerator::PointcloudGenerator(const rclcpp::NodeOptions &options)
    : Node("pointcloud_generator", options) {
  // params:
  this->declare_parameter("disparity_topic_name", "/stereo/disparity");
  this->declare_parameter("pointcloud_topic_name", "/pointcloud");
  this->declare_parameter("save_pcl", false);
  this->declare_parameter("time_profiling_dirpath", "");

  profiler_dirpath_ = this->get_parameter("time_profiling_dirpath").as_string();
  profiler_file_ = std::ofstream(profiler_dirpath_ + "profiler_pointcloud.txt",
                                 std::ios::out);
  if (!profiler_file_.is_open()) {
    RCLCPP_WARN(this->get_logger(),
                "Could not open profiler file at '%sprofiler_pointcloud.txt'. "
                "Check that the directory exists.",
                profiler_dirpath_.c_str());
  }
  // define reliable QoS :
  rclcpp::QoS reliable_qos(10);
  reliable_qos.reliable();
  // prepare the synchronizer and the callback registration
  uint32_t queue_size = 10;
  synchronizer_ = std::make_shared<message_filters::Synchronizer<ExactTime>>(
      ExactTime(queue_size));
  synchronizer_->registerCallback(
      std::bind(&PointcloudGenerator::pointcloudCallback, this,
                std::placeholders::_1, std::placeholders::_2));
  // publishers and subscribers:
  points3d_sub_.subscribe(this, "stereo/points3d",
                          reliable_qos.get_rmw_qos_profile());
  left_rectified_sub_.subscribe(this, "stereo/left/processed",
                                reliable_qos.get_rmw_qos_profile());
  synchronizer_->connectInput(left_rectified_sub_, points3d_sub_);

  pointcloud_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
      this->get_parameter("pointcloud_topic_name").as_string(), reliable_qos);
}

void PointcloudGenerator::pointcloudCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr &left_rectified_msg,
    const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg) {

  // left rectified setup---------------------->
  auto header = left_rectified_msg->header;
  auto start_img_conv = std::chrono::high_resolution_clock::now();
  left_rectified_shared_ = cv_bridge::toCvShare(left_rectified_msg, "rgb8");
  left_rectified_ = left_rectified_shared_->image;
  auto end_img_conv = std::chrono::high_resolution_clock::now();
  auto dur_ms_img_conv =
      std::chrono::duration<double, std::milli>(end_img_conv - start_img_conv)
          .count();
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Image conversion duration: " << dur_ms_img_conv << "ms"
                   << std::endl;
  }
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Image conversion duration: %.3f ms",
              dur_ms_img_conv);
#endif
  // left rectified setup <-----------------------

  // points3d setup ---------------------->
  auto start_points3d_conversion = std::chrono::high_resolution_clock::now();
  points3d_shared_ = cv_bridge::toCvShare(
      points3d_msg, sensor_msgs::image_encodings::TYPE_32FC4);
  cv::Mat points3D = points3d_shared_->image;
  auto end_points3d_conversion = std::chrono::high_resolution_clock::now();
  auto dur_ms_points3d_conv =
      std::chrono::duration<double, std::milli>(end_points3d_conversion -
                                                start_points3d_conversion)
          .count();
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Points3D msg conversion duration: "
                   << dur_ms_points3d_conv << "ms" << std::endl;
  }
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Points3d conversion duration: %.3f ms",
              dur_ms_points3d_conv);
#endif
  // points3D setup <------------------------
  // Create now the pointcloud msg
  sensor_msgs::msg::PointCloud2 cloud_msg;
  int height = left_rectified_.rows;
  int width = left_rectified_.cols;
  auto start_msg_setup = std::chrono::high_resolution_clock::now();
  sensor_msgs::PointCloud2Modifier modifier(cloud_msg);
  modifier.setPointCloud2Fields(4, "x", 1,
                                sensor_msgs::msg::PointField::FLOAT32, "y", 1,
                                sensor_msgs::msg::PointField::FLOAT32, "z", 1,
                                sensor_msgs::msg::PointField::FLOAT32, "rgb", 1,
                                sensor_msgs::msg::PointField::FLOAT32);

  modifier.resize(width * height);
  cloud_msg.height = left_rectified_.rows;
  cloud_msg.width = left_rectified_.cols;
  cloud_msg.row_step = cloud_msg.width * cloud_msg.point_step;
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "cloud_msg.row_step: %d", cloud_msg.row_step);
  RCLCPP_INFO(this->get_logger(), "cloud_msg.point_step: %d",
              cloud_msg.point_step);
#endif

  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");
  sensor_msgs::PointCloud2Iterator<uint32_t> iter_rgb(cloud_msg, "rgb");
  auto end_msg_setup = std::chrono::high_resolution_clock::now();
  auto dur_ms_msg_setup =
      std::chrono::duration<double, std::milli>(end_msg_setup - start_msg_setup)
          .count();
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "PointCloud2 message setup duration: " << dur_ms_msg_setup
                   << "ms" << std::endl;
  }
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "PointCloud2 message setup duration: %.3f ms",
              dur_ms_msg_setup);
#endif

  // iterate over points3D and fill the pointcloud msg
  auto start_iteration = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      const cv::Vec4f &pt = points3D.at<cv::Vec4f>(i, j);
      const cv::Vec3b &color = left_rectified_.at<cv::Vec3b>(i, j);
      // Fill the pointcloud msg with the 3D points :
      if (pt[0] == 0.0f && pt[1] == 0.0f && pt[2] == 0.0f) {
        *iter_x = std::numeric_limits<float>::quiet_NaN();
        *iter_y = std::numeric_limits<float>::quiet_NaN();
        *iter_z = std::numeric_limits<float>::quiet_NaN();
      } else {
        *iter_x = pt[0];
        *iter_y = pt[1];
        *iter_z = pt[2];
      }
      *iter_rgb = (static_cast<uint32_t>(color[0] << 16)) |
                  (static_cast<uint32_t>(color[1] << 8)) |
                  (static_cast<uint32_t>(color[2]));
      ++iter_x;
      ++iter_y;
      ++iter_z;
      ++iter_rgb;
    }
  }
  auto end_iteration = std::chrono::high_resolution_clock::now();
  auto dur_ms_iteration =
      std::chrono::duration<double, std::milli>(end_iteration - start_iteration)
          .count();
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Pointcloud iteration duration: " << dur_ms_iteration
                   << "ms" << std::endl;
  }
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Pointcloud iteration duration: %.3f ms",
              dur_ms_iteration);
#endif
  cloud_msg.header.stamp = this->get_clock()->now();
  cloud_msg.header.frame_id = "left_rectified";
  auto start_publish = std::chrono::high_resolution_clock::now();
  pointcloud_pub_->publish(cloud_msg);
  auto end_publish = std::chrono::high_resolution_clock::now();
  auto dur_ms_publish =
      std::chrono::duration<double, std::milli>(end_publish - start_publish)
          .count();
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "Publish duration: " << dur_ms_publish << "ms"
                   << std::endl;
  }
#if DEBUG_MODE
  RCLCPP_INFO(this->get_logger(), "Publish duration: %.3f ms", dur_ms_publish);
#endif

  // optionally save the pointcloud using PCL :
  if (this->get_parameter("save_pcl").as_bool() && save_counter_ < MAX_SAVES) {
    std::string pcl_filename =
        "pointcloud_" + std::to_string(save_counter_++) + ".pcd";
    pcl::PointCloud<pcl::PointXYZRGB> pcl_cloud;
    pcl::fromROSMsg(cloud_msg, pcl_cloud);
    pcl::io::savePCDFileASCII(pcl_filename, pcl_cloud);
    RCLCPP_INFO(this->get_logger(), "Saved pointcloud to %s",
                pcl_filename.c_str());
  }
  if (profiler_writes_count_ < 50) {
    profiler_file_ << "---------------------------------------------"
                   << "----------------------" << std::endl;
    profiler_writes_count_++;
  }
}

} // namespace pointcloud_pkg

#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(pointcloud_pkg::PointcloudGenerator)

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  rclcpp::init(argc, argv);
  std::shared_ptr<pointcloud_pkg::PointcloudGenerator> node =
      std::make_shared<pointcloud_pkg::PointcloudGenerator>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
