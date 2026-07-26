#include "stereo_rectifier.hpp"
#include <opencv2/calib3d.hpp>
#include <opencv2/core/hal/interface.h>

StereoRectifier::StereoRectifier(StereoCalibrationData calib_data)
    : calib_data_(calib_data) {
  std::cout << "Created StereoRectifier\n";
}

void StereoRectifier::rectify() {
  double alpha = 0.0; // crop the image
  auto flags = cv::CALIB_ZERO_DISPARITY;
  cv::stereoRectify(calib_data_.K_left, calib_data_.D_left, calib_data_.K_right,
                    calib_data_.D_right, calib_data_.image_size, calib_data_.R,
                    calib_data_.T, R1_, R2_, P1_, P2_, Q_, flags, alpha);
  // left maps generation :
  cv::initUndistortRectifyMap(calib_data_.K_left, calib_data_.D_left, R1_, P1_,
                              calib_data_.image_size, CV_32FC1, left_map1_,
                              left_map2_);
  // right maps generation ;
  cv::initUndistortRectifyMap(calib_data_.K_right, calib_data_.D_right, R2_,
                              P2_, calib_data_.image_size, CV_32FC1,
                              right_map1_, right_map2_);
  is_rectified_ = true;
}

std::vector<cv::Mat> StereoRectifier::getLeftMaps() {
  assert(is_rectified_ && "Call rectify() before getting the maps");
  return {left_map1_, left_map2_};
}

std::vector<cv::Mat> StereoRectifier::getRightMaps() {
  assert(is_rectified_ && "Call rectify() before getting the maps");
  return {right_map1_, right_map2_};
}

int main() { return 0; }
