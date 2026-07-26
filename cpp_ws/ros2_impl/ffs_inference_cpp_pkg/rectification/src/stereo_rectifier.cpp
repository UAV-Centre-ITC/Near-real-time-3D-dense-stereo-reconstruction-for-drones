#include "stereo_rectifier.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

StereoRectifier::StereoRectifier(StereoIntrinsics calib_data,
                                 bool write_to_file)
    : calib_data_(calib_data), write_to_file_(write_to_file) {
  std::cout << "Created StereoRectifier\n";
}

// Call this before calculateMaps() in case the images will be resized.
// DO NOT call this if you rectify on the original image size and then resize!
void StereoRectifier::scaleIntrinsics(double scale_factor) {
  calib_data_.image_size.height *= scale_factor;
  calib_data_.image_size.width *= scale_factor;
  calib_data_.K_left.at<double>(0, 0) *= scale_factor; // fx
  calib_data_.K_left.at<double>(1, 1) *= scale_factor; // fy
  calib_data_.K_left.at<double>(0, 2) *= scale_factor; // cx
  calib_data_.K_left.at<double>(1, 2) *= scale_factor; // cy

  calib_data_.K_right.at<double>(0, 0) *= scale_factor;
  calib_data_.K_right.at<double>(1, 1) *= scale_factor;
  calib_data_.K_right.at<double>(0, 2) *= scale_factor;
  calib_data_.K_right.at<double>(1, 2) *= scale_factor;
}

void StereoRectifier::calculateMaps(const StereoExtrinsics &extrinsics) {
  double alpha = 0;
  auto flags = cv::CALIB_ZERO_DISPARITY;
  // Compute the relative rotation and translation vectors between the two
  // cameras :

  // OpenCV camera axis convention : x-right, y-down, z-forward
  // print the matrices to verify :
  // std::cout << "R_left:\n" << extrinsics.R_left << std::endl;
  // std::cout << "T_left:\n" << extrinsics.T_left << std::endl;
  cv::Mat R1_cv = extrinsics.R_left;
  cv::Mat R2_cv = extrinsics.R_right;
  cv::Mat T1_cv = extrinsics.T_left;
  cv::Mat T2_cv = extrinsics.T_right;
  // R,T are expressed in the world frame. Based on that, the conversion
  // equations are :
  cv::Mat R_rel = R2_cv.t() * R1_cv;
  cv::Mat T_rel = R2_cv.t() * (T1_cv - T2_cv);
  // is_vertical_stereo_ = fabs(T_rel.at<double>(1)) >
  // fabs(T_rel.at<double>(0)); print the T_rel vector to see if we have a
  // vertical stereo setup (T_rel[1] should be close to 0): std::cout <<
  // "T_rel:\n" << T_rel << std::endl; std::cout << "R_rel:\n" << R_rel <<
  // std::endl; stereo rectification :
  cv::stereoRectify(calib_data_.K_left, calib_data_.D_left, calib_data_.K_right,
                    calib_data_.D_right, calib_data_.image_size, R_rel, T_rel,
                    R1_, R2_, P1_, P2_, Q_, flags, alpha);
  // print P matrices for debugging :
  // std::cout << "P1:\n" << P1_ << std::endl;
  // std::cout << "P2:\n" << P2_ << std::endl;
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

void StereoRectifier::calculateMaps(cv::Mat &R_rel, cv::Mat &T_rel) {
  double alpha = 0;
  auto flags = cv::CALIB_ZERO_DISPARITY;
  cv::stereoRectify(calib_data_.K_left, calib_data_.D_left, calib_data_.K_right,
                    calib_data_.D_right, calib_data_.image_size, R_rel, T_rel,
                    R1_, R2_, P1_, P2_, Q_, flags, alpha);
  cv::initUndistortRectifyMap(calib_data_.K_left, calib_data_.D_left, R1_, P1_,
                              calib_data_.image_size, CV_32FC1, left_map1_,
                              left_map2_);
  cv::initUndistortRectifyMap(calib_data_.K_right, calib_data_.D_right, R2_,
                              P2_, calib_data_.image_size, CV_32FC1,
                              right_map1_, right_map2_);

  is_rectified_ = true;
}

cv::Mat StereoRectifier::rectifyLeft(const cv::Mat &left_image) {
  assert(is_rectified_ && "Call calculateMaps() before rectifying images");
  cv::Mat rectified_left;

  cv::remap(left_image, rectified_left, left_map1_, left_map2_,
            cv::INTER_LINEAR);
  // convert to BGR for proper image saving (OpenCV saves in BGR format) :
  // write to file the rectified image  if specified

  // if (is_vertical_stereo_) {
  //   // We roate the image by 90 to make horizontally aligned images
  //   cv::rotate(rectified_left, rectified_left, cv::ROTATE_90_CLOCKWISE);
  // }
  if (write_to_file_) {
    // cv::imwrite("rectified_left.png", rectified_left);
    // write also the non-rectified image
    cv::imwrite("original_left_image.png", left_image);
  }
  return rectified_left;
}

cv::Mat StereoRectifier::rectifyRight(const cv::Mat &right_image) {
  assert(is_rectified_ && "Call calculateMaps() before rectifying images");
  cv::Mat rectified_right;

  cv::remap(right_image, rectified_right, right_map1_, right_map2_,
            cv::INTER_LINEAR);
  // if (is_vertical_stereo_) {
  //   // We roate the image by 90 to make horizontally aligned images
  //   cv::rotate(rectified_right, rectified_right, cv::ROTATE_90_CLOCKWISE);
  // }
  // write to file the rectified image  if specified
  if (write_to_file_) {
    // cv::imwrite("rectified_right.png", rectified_right);
    // write also the non-rectified image
    cv::imwrite("original_right_image.png", right_image);
  }
  return rectified_right;
}

cv::Mat StereoRectifier::getR1_rectified() {
  if (!is_rectified_) {
    throw std::runtime_error("Call calculateMaps() before getting R1");
  }
  return R1_;
}

cv::Mat StereoRectifier::getR2_rectified() {
  if (!is_rectified_) {
    throw std::runtime_error("Call calculateMaps() before getting R2");
  }
  return R2_;
}

// Use the new projection matrix of camera 2 (right) to get the baseline
// between the two cameras:
double StereoRectifier::getBaseline() {
  if (!is_rectified_) {
    throw std::runtime_error("Call calculateMaps() before getting baseline");
  }
  double baseline = std::abs(P2_.at<double>(0, 3) / P2_.at<double>(0, 0));
  return baseline;
}
