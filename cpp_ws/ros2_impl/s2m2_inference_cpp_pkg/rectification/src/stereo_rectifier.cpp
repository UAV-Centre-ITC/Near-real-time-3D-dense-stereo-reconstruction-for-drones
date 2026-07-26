#include "stereo_rectifier.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <ratio>

StereoRectifier::StereoRectifier(StereoIntrinsics calib_data,
                                 bool write_to_file)
    : calib_data_(calib_data), write_to_file_(write_to_file) {
  // std::cout << "Created StereoRectifier\n";
}

// Call this before calculateMaps() in case the images need to be resized before
// running the rectification. DO NOT call this if you rectify on the original
// image size and later resize!
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

void StereoRectifier::scaleQMatrix(double scale_factor) {
  Q_.at<double>(0, 3) *= scale_factor;
  Q_.at<double>(1, 3) *= scale_factor;
  Q_.at<double>(2, 3) *= scale_factor;
  Q_.at<double>(3, 3) *= scale_factor;
}
//
// inline std::vector<float> StereoRectifier::getQFloatVector() {
//   assert(is_rectified_ && "Call calculateMaps() before getting Q");
//   return std::vector<float>(Q_.begin(), Q_.end());
// }

cv::Mat StereoRectifier::getQMatrix() {
  // std::cout << "WARNING: ENSURE Q MATRIX HAS BEEN SCALED (IF NEEDED).\n";
  return Q_;
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

void StereoRectifier::calculateMaps(const StereoExtrinsics &extrinsics) {
  double alpha = 0; // crop the image
  auto flags = cv::CALIB_ZERO_DISPARITY;
  // Compute the relative rotation and translation vectors between the two
  // cameras :

  // OpenCV camera axis convention : x-right, y-down, z-forward
  // print the matrices to verify :
  // std::cout << "R_left:\n" << extrinsics.R_left << std::endl;
  // std::cout << "t_left:\n" << extrinsics.t_left << std::endl;
  cv::Mat R1_cv = extrinsics.R_left;
  cv::Mat R2_cv = extrinsics.R_right;
  cv::Mat T1_cv = extrinsics.t_left;
  cv::Mat T2_cv = extrinsics.t_right;
  // R,T are expressed in the camera frame from pix4dmapper. Based on that,
  // the conversion equations are : ⇒ R_rel = R2 R1^T And T_rel = T2 - R_rel *
  // T1 cv::Mat R_rel = R2_cv.t() * R1_cv; cv::Mat t_rel = R2_cv.t() * (T1_cv
  // - T2_cv);
  cv::Mat R_rel = R2_cv * R1_cv.t();
  cv::Mat t_rel = T2_cv - R_rel * T1_cv;
  auto start_rectify = std::chrono::high_resolution_clock::now();
  cv::stereoRectify(calib_data_.K_left, calib_data_.D_left, calib_data_.K_right,
                    calib_data_.D_right, calib_data_.image_size, R_rel, t_rel,
                    R1_, R2_, P1_, P2_, Q_, flags, alpha);
  auto end_rectify = std::chrono::high_resolution_clock::now();
  auto dur_rectify_ms =
      std::chrono::duration<double, std::milli>(end_rectify - start_rectify)
          .count();
  // std::cout << "stereoRectify() took " << dur_rectify_ms << " ms\n";
  // print P matrices for debugging :
  // std::cout << "P1:\n" << P1_ << std::endl;
  // std::cout << "P2:\n" << P2_ << std::endl;
  // left maps generation :
  auto start_left_map = std::chrono::high_resolution_clock::now();
  cv::initUndistortRectifyMap(calib_data_.K_left, calib_data_.D_left, R1_, P1_,
                              calib_data_.image_size, CV_32FC1, left_map1_,
                              left_map2_);
  auto end_left_map = std::chrono::high_resolution_clock::now();
  auto dur_left_map_ms =
      std::chrono::duration<double, std::milli>(end_left_map - start_left_map)
          .count();
  // std::cout << "initUndistortRectifyMap() took " << dur_left_map_ms << "
  // ms\n"; right maps generation ;
  auto start_right_map = std::chrono::high_resolution_clock::now();
  cv::initUndistortRectifyMap(calib_data_.K_right, calib_data_.D_right, R2_,
                              P2_, calib_data_.image_size, CV_32FC1,
                              right_map1_, right_map2_);
  auto end_right_map = std::chrono::high_resolution_clock::now();
  auto dur_right_map_ms =
      std::chrono::duration<double, std::milli>(end_right_map - start_right_map)
          .count();
  // std::cout << "initUndistortRectifyMap() took " << dur_right_map_ms << "
  // ms\n";

  is_rectified_ = true;
}

void StereoRectifier::calculateMaps(cv::Mat &R_rel, cv::Mat &t_rel) {
  double alpha = -1; // TODO: test this and revert back to 0
  auto flags = 0;
  cv::stereoRectify(calib_data_.K_left, calib_data_.D_left, calib_data_.K_right,
                    calib_data_.D_right, calib_data_.image_size, R_rel, t_rel,
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
  auto start_remapping = std::chrono::high_resolution_clock::now();
  cv::remap(left_image, rectified_left, left_map1_, left_map2_,
            cv::INTER_LINEAR);
  auto end_remapping = std::chrono::high_resolution_clock::now();
  auto dur_remapping_ms =
      std::chrono::duration<double, std::milli>(end_remapping - start_remapping)
          .count();
  // std::cout << "remapping left took: " << dur_remapping_ms << " ms\n";
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

  auto start_remapping = std::chrono::high_resolution_clock::now();
  cv::remap(right_image, rectified_right, right_map1_, right_map2_,
            cv::INTER_LINEAR);
  auto end_remapping = std::chrono::high_resolution_clock::now();
  auto dur_remapping_ms =
      std::chrono::duration<double, std::milli>(end_remapping - start_remapping)
          .count();
  // std::cout << "remapping right took: " << dur_remapping_ms << " ms\n";
  // write to file the rectified image  if specified
  if (write_to_file_) {
    // cv::imwrite("rectified_right.png", rectified_right);
    // write also the non-rectified image
    cv::imwrite("original_right_image.png", right_image);
  }
  return rectified_right;
}
