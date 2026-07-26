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
#include <vector>

class PointcloudGenerator : public rclcpp::Node {
public:
  /**
   * @brief Constructs the PointcloudGenerator, sets up subscribers for
   * left rectified images and 3D points, the synchronizer, and publisher.
   */
  PointcloudGenerator();
  /** @brief Default destructor. */
  ~PointcloudGenerator() = default;

private:
  using ExactTime =
      message_filters::sync_policies::ExactTime<sensor_msgs::msg::Image,
                                                sensor_msgs::msg::Image>;
  cv::Mat left_rectified_;
  std::string profiler_dirpath_;
  std::ofstream profiler_file_;
  int profiler_writes_count_ = 0;
  int map_counter_ = 0;
  int generation_counter_ = 0;
  const size_t DEPTHMAPS_NUM = 12; // 3 depthmaps * 4 patches each one
  std::vector<sensor_msgs::msg::Image::ConstSharedPtr> depthmap_msg_buffer_;
  std::vector<sensor_msgs::msg::Image::ConstSharedPtr> left_image_msg_buffer_;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;

  message_filters::Subscriber<sensor_msgs::msg::Image> points3d_sub_;
  message_filters::Subscriber<sensor_msgs::msg::Image> left_rectified_sub_;
  std::shared_ptr<message_filters::Synchronizer<ExactTime>> synchronizer_;
  /**
   * @brief Synchronized callback that buffers incoming pairs of left rectified
   * image and 3D points messages. Once DEPTHMAPS_NUM pairs are collected, it
   * triggers point cloud generation.
   * @param left_rectified_msg The rectified left camera image.
   * @param points3d_msg The 3D points (homogeneous coordinates) message.
   */
  void pointcloudCallback(
      const sensor_msgs::msg::Image::ConstSharedPtr &left_rectified_msg,
      const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg);
  /**
   * @brief Generates and publishes PointCloud2 messages from all buffered
   * depth maps. Iterates over each pair, converts 3D points to a point cloud
   * with RGB color from the left rectified image, and publishes each one.
   */
  void generatePointcloud();
};
