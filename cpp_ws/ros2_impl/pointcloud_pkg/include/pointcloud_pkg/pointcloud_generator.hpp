#include "builtin_interfaces/msg/time.hpp"
#include "cv_bridge/cv_bridge.h"
#include "message_filters/sync_policies/exact_time.hpp"
#include "message_filters/synchronizer.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <message_filters/subscriber.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/matx.hpp>
#include <opencv2/opencv.hpp>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>

class PointcloudGenerator : public rclcpp::Node {
public:
  PointcloudGenerator();
  ~PointcloudGenerator() = default;

private:
  using ExactTime =
      message_filters::sync_policies::ExactTime<sensor_msgs::msg::Image,
                                                sensor_msgs::msg::Image>;
  cv::Mat left_rectified_;
  std::string profiler_dirpath_;
  std::ofstream profiler_file_;
  int profiler_writes_count_ = 0;
  int save_counter_ = 0;
  const int MAX_SAVES = 12;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;

  message_filters::Subscriber<sensor_msgs::msg::Image> points3d_sub_;
  message_filters::Subscriber<sensor_msgs::msg::Image> left_rectified_sub_;

  std::shared_ptr<message_filters::Synchronizer<ExactTime>> synchronizer_;
  void pointcloudCallback(
      const sensor_msgs::msg::Image::ConstSharedPtr &left_rectified_msg,
      const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg);
};
