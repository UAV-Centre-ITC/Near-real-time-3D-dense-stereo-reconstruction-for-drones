// intrinsics:
#pragma once
#include "calibration_data.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

// Here we have only one camera. So we use the same intrinsics for left and
// right.
const cv::Size img_size{2448, 2048};
// const cv::Mat K_left = (cv::Mat_<double>(3, 3) << 1443.43, 0, 1179.5, 0,
//                         1444.34, 1044.9, 0.0, 0.0, 1.0);
// const cv::Mat K_right = K_left;

// Pix4d intrinsics :
//  1457.75750480182205137680 0 1175.26010982729053466755
//  0 1457.75750480182205137680 1042.45094638104637851939
//  0 0 1

const cv::Mat K_left = (cv::Mat_<double>(3, 3) << 1457.75750480182205137680, 0,
                        1175.26010982729053466755, 0, 1457.75750480182205137680,
                        1042.45094638104637851939, 0, 0, 1);

const cv::Mat K_right = K_left;

// pix4d distortion:
//  -0.03977108672537384149 0.09886022505670410965 -0.04821385243386436953
//  0.00117026804208919489 0.00037890920457606900
//

// from SLAM :
//  const cv::Mat D_left =
//      (cv::Mat_<double>(1, 5) << -0.0560, 0.1180, 0.00122, -0.00064, -0.0627);

// coefficient order : k1,k2,p1,p2,k3 :
const cv::Mat D_left =
    (cv::Mat_<double>(1, 5) << -0.03977108672537384149, 0.09886022505670410965,
     0.00117026804208919489, 0.00037890920457606900, -0.04821385243386436953);

const cv::Mat D_right = D_left;

const StereoIntrinsics STEREO_INTRINSICS{K_left, K_right, D_left, D_right,
                                         img_size};
