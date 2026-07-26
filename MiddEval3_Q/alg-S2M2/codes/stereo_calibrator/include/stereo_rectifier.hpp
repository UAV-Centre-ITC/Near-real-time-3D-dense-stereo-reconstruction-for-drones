#include "calibration_data.hpp"
#include <iostream>

class StereoRectifier {
public:
  StereoRectifier(StereoCalibrationData calib_data);
  void rectify();
  std::vector<cv::Mat> getLeftMaps();
  std::vector<cv::Mat> getRightMaps();
  cv::Mat getQMatrix() const { return Q_; }

private:
  StereoCalibrationData calib_data_;
  cv::Mat R1_, R2_, P1_, P2_, Q_; // outputs from stereoRectify
  cv::Mat left_map1_, left_map2_;
  cv::Mat right_map1_, right_map2_;
  bool is_rectified_ = false;
};
