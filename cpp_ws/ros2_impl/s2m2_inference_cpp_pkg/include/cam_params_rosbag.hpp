// intrinsics:
#pragma once
#include "calibration_data.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

// Here we have only one camera. So we use the same intrinsics for left and
// right.
const cv::Size img_size{2448, 2048}; // width, height
const cv::Mat K_left = (cv::Mat_<double>(3, 3) << 1443.43, 0, 1179.5, 0,
                        1444.34, 1044.9, 0.0, 0.0, 1.0);
const cv::Mat K_right = K_left;

const cv::Mat D_left =
    (cv::Mat_<double>(1, 5) << -0.0560, 0.1180, 0.00122, -0.00064, -0.0627);

const cv::Mat D_right = D_left;

const StereoIntrinsics STEREO_INTRINSICS{K_left, K_right, D_left, D_right,
                                         img_size};
