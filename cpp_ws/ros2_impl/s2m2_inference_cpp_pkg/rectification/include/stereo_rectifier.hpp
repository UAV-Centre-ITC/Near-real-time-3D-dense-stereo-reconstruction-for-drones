#pragma once
#include "calibration_data.hpp"
#include <chrono>

class StereoRectifier {
public:
  /**
   * @brief Constructs a StereoRectifier with given intrinsic calibration data.
   * @param calib_data Intrinsic parameters for left and right cameras.
   * @param write_to_file If true, saves original images during rectification.
   */
  StereoRectifier(StereoIntrinsics calib_data, bool write_to_file = false);

  /**
   * @brief Scales camera intrinsics (fx, fy, cx, cy) and image size by a
   * factor.
   * @param scale_factor The scaling factor to apply.
   */
  void scaleIntrinsics(double scale_factor);
  /**
   * @brief Scales the Q matrix translation components by a factor.
   * @param scale_factor The scaling factor to apply.
   */
  void scaleQMatrix(double scale_factor);
  /**
   * @brief Computes rectification maps from stereo extrinsic parameters.
   * @param extrinsics Extrinsic parameters for the stereo pair.
   */
  void calculateMaps(const StereoExtrinsics &extrinsics);
  /**
   * @brief Computes rectification maps from explicit relative R and T matrices.
   * @param R_rel Relative rotation matrix between cameras.
   * @param T_rel Relative translation vector between cameras.
   */
  void calculateMaps(cv::Mat &R_rel, cv::Mat &T_rel);
  /**
   * @brief Rectifies the left image using the precomputed map.
   * @param left_img Input left camera image.
   * @return Rectified left image.
   */
  [[nodiscard]] cv::Mat rectifyLeft(const cv::Mat &left_img);
  /**
   * @brief Rectifies the right image using the precomputed map.
   * @param right_img Input right camera image.
   * @return Rectified right image.
   */
  [[nodiscard]] cv::Mat rectifyRight(const cv::Mat &right_img);
  /**
   * @brief Returns the Q matrix for reprojecting disparity to 3D.
   * @return The 4x4 Q matrix.
   */
  cv::Mat getQMatrix();
  /**
   * @brief Returns the rectification rotation matrix for the left camera.
   * @return The 3x3 rotation matrix R1.
   */
  cv::Mat getR1_rectified();
  /**
   * @brief Returns the rectification rotation matrix for the right camera.
   * @return The 3x3 rotation matrix R2.
   */
  cv::Mat getR2_rectified();
  /**
   * @brief Computes the baseline distance between the rectified cameras.
   * @return Baseline distance in world units.
   */
  double getBaseline();
  /**
   * @brief Returns the Q matrix as a flat vector of floats.
   * @return Q matrix in row-major float vector format.
   */
  inline std::vector<float> getQFloatVector() const;

private:
  StereoIntrinsics calib_data_;
  bool write_to_file_;
  cv::Mat R1_, R2_, P1_, P2_, Q_; // outputs from stereoRectify
  cv::Mat left_map1_, left_map2_;
  cv::Mat right_map1_, right_map2_;
  bool is_rectified_ = false;
};
