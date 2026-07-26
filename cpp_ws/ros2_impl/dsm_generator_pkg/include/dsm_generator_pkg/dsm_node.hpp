#include <cv_bridge/cv_bridge.h>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
/*
 The idea for constructing the DSM is to first find the maximum
 and minimum X,Y values across all depthmaps. Then, we can
 create the edges of the DSM based on those. Then, we simply
 iterate over the depthmaps and perform that quantization for
 populating the DSM.
 */

class DSMNode : public rclcpp::Node {
public:
  DSMNode();

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depthmap_sub_;
  int dsm_width_;
  int dsm_height_;
  int depthmaps_per_dsm_;
  int depthmaps_counter_ = 0;
  bool save_dsm_;
  std::vector<sensor_msgs::msg::Image::ConstSharedPtr> depthmaps_buffer_;
  cv::Mat dsm_;
  cv::Mat dsm_edges_;
  float step_X_;
  float step_Y_;
  float min_X_; // for .tfw file
  float max_Y_; // for .tfw file

  // methods:
  void
  depthmapCallback(const sensor_msgs::msg::Image::ConstSharedPtr &points3d_msg);
  void findDSMEdges();
  void generateDSM();
  void saveDsmToPng(const std::string &filename);
  void saveDsmToTif();

  // helper :
  const cv::Mat
  cvMatFromImageMsg(const sensor_msgs::msg::Image::ConstSharedPtr &msg);
};
