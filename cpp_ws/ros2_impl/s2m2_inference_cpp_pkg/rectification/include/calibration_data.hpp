#pragma once
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

struct StereoIntrinsics {
  cv::Mat K_left;  // Intrinsic camera matrix for the left camera
  cv::Mat K_right; // Intrinsic camera matrix for the right camera
  cv::Mat D_left;  // Distortion coeffs 1x5
  cv::Mat D_right;
  cv::Size image_size;
  StereoIntrinsics(const cv::Mat &K_l, const cv::Mat &K_r, const cv::Mat &D_l,
                   const cv::Mat &D_r, const cv::Size &img_size)
      : K_left(K_l), K_right(K_r), D_left(D_l), D_right(D_r),
        image_size(img_size) {}
};

// Extrinsic parameters of the left and right cameras
struct StereoExtrinsics {
  cv::Mat R_left;
  cv::Mat T_left;
  cv::Mat R_right;
  cv::Mat T_right;
  StereoExtrinsics(const cv::Mat &R_l, const cv::Mat &T_l, const cv::Mat &R_r,
                   const cv::Mat &T_r)
      : R_left(R_l), T_left(T_l), R_right(R_r), T_right(T_r) {}
};
