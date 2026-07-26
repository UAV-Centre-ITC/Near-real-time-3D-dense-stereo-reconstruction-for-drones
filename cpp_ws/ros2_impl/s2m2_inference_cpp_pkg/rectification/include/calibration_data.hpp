#pragma once
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

/** @brief Stores intrinsic calibration data for a stereo camera pair. */
struct StereoIntrinsics {
  cv::Mat K_left;   //!< Intrinsic camera matrix (3x3) for the left camera
  cv::Mat K_right;  //!< Intrinsic camera matrix (3x3) for the right camera
  cv::Mat D_left;   //!< Distortion coefficients (1x5) for the left camera
  cv::Mat D_right;  //!< Distortion coefficients (1x5) for the right camera
  cv::Size image_size; //!< Image dimensions (width x height)
  /**
   * @brief Constructs StereoIntrinsics from left/right K and D matrices.
   * @param K_l Left camera intrinsic matrix (3x3).
   * @param K_r Right camera intrinsic matrix (3x3).
   * @param D_l Left camera distortion coefficients (1x5).
   * @param D_r Right camera distortion coefficients (1x5).
   * @param img_size Image dimensions.
   */
  StereoIntrinsics(const cv::Mat &K_l, const cv::Mat &K_r, const cv::Mat &D_l,
                   const cv::Mat &D_r, const cv::Size &img_size)
      : K_left(K_l), K_right(K_r), D_left(D_l), D_right(D_r),
        image_size(img_size) {}
};

/** @brief Stores extrinsic pose data (rotation + translation) for left and right cameras. */
struct StereoExtrinsics {
  cv::Mat R_left;   //!< Rotation matrix (3x3) of the left camera (world to camera)
  cv::Mat t_left;   //!< Translation vector (3x1) of the left camera
  cv::Mat R_right;  //!< Rotation matrix (3x3) of the right camera (world to camera)
  cv::Mat t_right;  //!< Translation vector (3x1) of the right camera
  /**
   * @brief Constructs StereoExtrinsics from left/right R and t matrices.
   * @param R_l Left camera rotation matrix (3x3).
   * @param t_l Left camera translation vector (3x1).
   * @param R_r Right camera rotation matrix (3x3).
   * @param t_r Right camera translation vector (3x1).
   */
  StereoExtrinsics(const cv::Mat &R_l, const cv::Mat &t_l, const cv::Mat &R_r,
                   const cv::Mat &t_r)
      : R_left(R_l), t_left(t_l), R_right(R_r), t_right(t_r) {}
};
