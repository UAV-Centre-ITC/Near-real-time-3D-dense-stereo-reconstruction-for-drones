#include <string>
#define DEBUG_MODE true
#include "pointcloud_pkg/pointcloud_generator.hpp"

PointcloudGenerator::PointcloudGenerator() : Node("pointcloud_generator") {
  // params:
  this->declare_parameter("disparity_topic_name", "/stereo/disparity");
  this->declare_parameter("pointcloud_topic_name", "/pointcloud");
  this->declare_parameter("save_pcl", false);
  this->declare_parameter("time_profiling_dirpath", "");

  profiler_dirpath_ = this->get_parameter("time_profiling_dirpath").as_string();
  profiler_file_ = std::ofstream(profiler_dirpath_ + "profiler_pointcloud.txt",
                                 std::ios::out);
  // define reliable QoS :
  rclcpp::QoS reliable_qos(5);
  reliable_qos.reliable();
  // prepare the synchronizer and the callback registration
  uint32_t queue_size = 5;
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
  depthmap_msg_buffer_.reserve(DEPTHMAPS_NUM);
  left_image_msg_buffer_.reserve(DEPTHMAPS_NUM);
}

void PointcloudGenerator::pointcloudCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr &left_rectified_msg,
    const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg) {
  depthmap_msg_buffer_.push_back(points3d_msg);
  left_image_msg_buffer_.push_back(left_rectified_msg);
  map_counter_++;
  if (static_cast<size_t>(map_counter_) == DEPTHMAPS_NUM) {
    generatePointcloud();
  }
}

void PointcloudGenerator::generatePointcloud() {
  auto start_total = std::chrono::high_resolution_clock::now();
  pcl::PointCloud<pcl::PointXYZRGB> pcl_complete;
  for (size_t i = 0; i < DEPTHMAPS_NUM; i++) {
    auto left_rectified_msg = left_image_msg_buffer_[i];
    auto points3d_msg = depthmap_msg_buffer_[i];
    // left rectified setup---------------------->
    auto header = left_rectified_msg->header;
    left_rectified_ = cv_bridge::toCvShare(left_rectified_msg, "rgb8")->image;
    // left rectified setup <-----------------------

    // points3d setup ---------------------->
    auto start_points3d_conversion = std::chrono::high_resolution_clock::now();
    cv_bridge::CvImageConstPtr points3D_ptr;
    points3D_ptr = cv_bridge::toCvShare(
        points3d_msg, sensor_msgs::image_encodings::TYPE_32FC4);
    const cv::Mat points3D = points3D_ptr->image;
#if DEBUG_MODE
    RCLCPP_WARN(this->get_logger(), "Total bytes of points3d matrix: %d",
                points3D.total() * points3D.elemSize());
#endif
    // points3D setup <------------------------
#if DEBUG_MODE
    auto end_points3d_conversion = std::chrono::high_resolution_clock::now();
    auto dur_ms_points3d_conv =
        std::chrono::duration<double, std::milli>(end_points3d_conversion -
                                                  start_points3d_conversion)
            .count();
    RCLCPP_INFO(this->get_logger(), "Points3d conversion duration: %.3f ms",
                dur_ms_points3d_conv);

    // if (profiler_writes_count_ < 50) {
    //   profiler_file_ << "Points3D msg conversion duration: "
    //                  << dur_ms_points3d_conv << "ms" << std::endl;
    // }
#endif
    // Create now the pointcloud msg
    sensor_msgs::msg::PointCloud2 cloud_msg;
    int height = left_rectified_.rows;
    int width = left_rectified_.cols;
    sensor_msgs::PointCloud2Modifier modifier(cloud_msg);
    modifier.setPointCloud2Fields(4, "x", 1,
                                  sensor_msgs::msg::PointField::FLOAT32, "y", 1,
                                  sensor_msgs::msg::PointField::FLOAT32, "z", 1,
                                  sensor_msgs::msg::PointField::FLOAT32, "rgb",
                                  1, sensor_msgs::msg::PointField::FLOAT32);

    modifier.resize(width * height);
    cloud_msg.height = left_rectified_.rows;
    cloud_msg.width = left_rectified_.cols;
    cloud_msg.row_step = cloud_msg.width * cloud_msg.point_step;
#if DEBUG_MODE
    RCLCPP_INFO(this->get_logger(), "cloud_msg.row_step: %d",
                cloud_msg.row_step);
    RCLCPP_INFO(this->get_logger(), "cloud_msg.point_step: %d",
                cloud_msg.point_step);
#endif

    sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
    sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
    sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");
    sensor_msgs::PointCloud2Iterator<uint32_t> iter_rgb(cloud_msg, "rgb");

    // iterate over points3D and fill the pointcloud msg
    auto start_iteration = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        const cv::Vec4f &pt = points3D.at<cv::Vec4f>(i, j);
        // if invalid point, skip:
        // if (pt[0] == 0.f && pt[1] == 0.f && pt[2] == 0.f) {
        //   continue;
        // }
        const cv::Vec3b &color = left_rectified_.at<cv::Vec3b>(i, j);
        // Fill the pointcloud msg with the 3D points :
        *iter_x = pt[0];
        *iter_y = pt[1];
        *iter_z = pt[2];
        *iter_rgb = (static_cast<uint32_t>(color[2] << 16)) |
                    (static_cast<uint32_t>(color[1] << 8)) |
                    (static_cast<uint32_t>(color[0]));
        ++iter_x;
        ++iter_y;
        ++iter_z;
        ++iter_rgb;
      }
    }
#if DEBUG_MODE
    auto end_iteration = std::chrono::high_resolution_clock::now();
    auto dur_ms_iteration = std::chrono::duration<double, std::milli>(
                                end_iteration - start_iteration)
                                .count();
    RCLCPP_INFO(this->get_logger(), "Pointcloud iteration duration: %.3f ms",
                dur_ms_iteration);
    // if (profiler_writes_count_ < 50) {
    //   profiler_file_ << "Pointcloud iteration duration: " << dur_ms_iteration
    //                  << "ms" << std::endl;
    // }
#endif
    cloud_msg.header.stamp = this->get_clock()->now();
    cloud_msg.header.frame_id = "left_rectified";
    pointcloud_pub_->publish(cloud_msg);
    std::string pcl_filename;
    // concatenate pointcloud if needed
    if (this->get_parameter("save_pcl").as_bool()) {
      pcl::PointCloud<pcl::PointXYZRGB> pcl_cloud;
      pcl::fromROSMsg(cloud_msg, pcl_cloud);
      pcl_filename = "pointcloud_" + std::to_string(i) + ".pcd";
      pcl::io::savePCDFileASCII(pcl_filename, pcl_cloud);
    }
#if DEBUG_MODE
    RCLCPP_INFO(this->get_logger(), "Saved pointcloud to %s",
                pcl_filename.c_str());
#endif
    // if (profiler_writes_count_ < 50) {
    //   profiler_file_ << "---------------------------------------------"
    //                  << "----------------------" << std::endl;
    //   profiler_writes_count_++;
    //   }
    auto end_total = std::chrono::high_resolution_clock::now();
    auto dur_ms_total =
        std::chrono::duration<double, std::milli>(end_total - start_total)
            .count();
    generation_counter_++;
    if (profiler_writes_count_ < 50) {
      profiler_file_ << "Pointcloud generation " << generation_counter_
                     << " duration: " << dur_ms_total << "ms" << std::endl;
      profiler_writes_count_++;
    }
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  rclcpp::init(argc, argv);
  std::shared_ptr<PointcloudGenerator> node =
      std::make_shared<PointcloudGenerator>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
