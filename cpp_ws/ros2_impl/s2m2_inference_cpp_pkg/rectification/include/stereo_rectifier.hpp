#pragma once
#include "calibration_data.hpp"

class StereoRectifier {
public:
  StereoRectifier(StereoIntrinsics calib_data);
  void scaleIntrinsics(double scale_factor);
  void calculateMaps(const StereoExtrinsics &extrinsics);
  void calculateMaps(cv::Mat &R_rel, cv::Mat &T_rel);
  [[nodiscard]] cv::Mat rectifyLeft(const cv::Mat &left_img);
  [[nodiscard]] cv::Mat rectifyRight(const cv::Mat &right_img);
  [[nodiscard]] cv::Mat getR1_rectified();
  [[nodiscard]] cv::Mat getR2_rectified();
  [[nodiscard]] cv::Mat getQMatrix() { return Q_; }
  double getBaseline();

private:
  StereoIntrinsics calib_data_;
  cv::Mat R1_, R2_, P1_, P2_,
      Q_; // outputs from stereoRectify
  cv::Mat left_map1_, left_map2_;
  cv::Mat right_map1_, right_map2_;
  // Define the OpenCV flip matrix
  bool is_rectified_ = false;
  // bool is_vertical_stereo_ = false;
};
