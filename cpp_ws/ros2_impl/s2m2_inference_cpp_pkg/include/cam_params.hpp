#pragma once
#include "calibration_data.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

const cv::Size img_size{4592, 3448};
const cv::Mat K_left = (cv::Mat_<double>(3, 3) << 3837.0471, 0, 2287.7796, 0,
                        3837.0471, 1742.8397, 0, 0, 1);
const cv::Mat K_right = (cv::Mat_<double>(3, 3) << 3837.0471, 0, 2287.7796, 0,
                         3837.0471, 1742.8397, 0, 0, 1);

const cv::Mat D_left =
    (cv::Mat_<double>(1, 5) << -0.0493, 0.0424, 0.00028, -0.00085, -0.0292);
const cv::Mat D_right =
    (cv::Mat_<double>(1, 5) << -0.0493, 0.0424, 0.00028, -0.00085, -0.0292);

// extrinsics :
cv::Mat R2_world2cam = (cv::Mat_<double>(3, 3) << -0.3024, -0.9520, 0.0466,
                        -0.9506, 0.3048, 0.0583, -0.0697, -0.0267, -0.9972);
cv::Mat Cw_2 = (cv::Mat_<double>(3, 1) << 53.8105, 2.7345, 109.1574);
cv::Mat R1_world2cam = (cv::Mat_<double>(3, 3) << -0.3004, -0.9471, 0.1133,
                        -0.9482, 0.3093, 0.0714, -0.1026, -0.0860, -0.9910);
cv::Mat Cw_1 = (cv::Mat_<double>(3, 1) << 63.7793, 21.5463, 109.2223);

// Calculate the translation vectors from world to camera,
// T = -R * Cw
cv::Mat T1_world2cam = -R1_world2cam * Cw_1;
cv::Mat T2_world2cam = -R2_world2cam * Cw_2;

// fix the vaiable names :
const StereoIntrinsics STEREO_INTRINSICS{K_left, K_right, D_left, D_right,
                                         img_size};
const StereoExtrinsics STEREO_EXTRINSICS{R1_world2cam, T1_world2cam,
                                         R2_world2cam, T2_world2cam};
// TODO : Extrnisics will later not be constant, but will be received as ros2
// msg.
