#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

struct StereoCalibrationData {
  cv::Mat K_left;  // Intrinsic camera matrix for the left camera
  cv::Mat K_right; // Intrinsic camera matrix for the right camera
  cv::Mat D_left;  // Distortion coeffs 1x5
  cv::Mat D_right;
  cv::Mat T; // Translation from the left coord system to the right
  cv::Mat R; // Rotation from the left coord system to the right
  cv::Size image_size;
};
