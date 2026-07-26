#pragma once
#include "calibration_data.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

/** @brief Image dimensions (width x height) for the stereo camera setup. */
const cv::Size IMG_SIZE{4592, 3448};
/** @brief Intrinsic camera matrix K for the left camera (3x3). */
const cv::Mat K_LEFT = (cv::Mat_<double>(3, 3) << 3837.04710692373510028119, 0,
                        2287.77960528229596093297, 0, 3837.04710692373510028119,
                        1742.83970496531173921539, 0, 0, 1);
/** @brief Intrinsic camera matrix K for the right camera (identical to left).
 */
const cv::Mat K_RIGHT = K_LEFT.clone();

/** @brief Distortion coefficients (k1, k2, p1, p2, k3) for the left camera. */
const cv::Mat D_LEFT =
    (cv::Mat_<double>(1, 5) << -0.04930618424089858870, 0.04239222797251372687,
     0.00028446432839522286, -0.00085023797648524325, -0.02928533838596112174);
/** @brief Distortion coefficients for the right camera (identical to left). */
const cv::Mat D_RIGHT = D_LEFT.clone();

const cv::Mat PIX4D_OFFSET_VEC =
    (cv::Mat_<float>(3, 1) << 384425.000, 5708777.000, 123.00);

/** @brief Combined stereo intrinsic parameters (K and D for both cameras +
 * image size). */
const StereoIntrinsics STEREO_INTRINSICS{K_LEFT, K_RIGHT, D_LEFT, D_RIGHT,
                                         IMG_SIZE};

// Image P1000759
/** @brief Rotation matrix for image P1000759 (world to camera). */
const cv::Mat R_759 =
    (cv::Mat_<double>(3, 3) << 0.36334151502688549762, 0.93160595842804982958,
     0.00965824411176458356, 0.92787106063518343113, -0.36277946239770286763,
     0.08629227368758733696, 0.08389420894023209840, -0.02239196024890610648,
     -0.99622304822891150078);
/** @brief Camera center in world coordinates for image P1000759. */
const cv::Mat Cw_759 = (cv::Mat_<double>(3, 1) << 44.80619498845744175242,
                        35.36773974479656601488, 107.67086779851349831461);
/** @brief Translation vector for image P1000759 (computed as -R * Cw). */
const cv::Mat T_759 = -R_759 * Cw_759;

// Image P1000760
/** @brief Rotation matrix for image P1000760 (world to camera). */
const cv::Mat R_760 =
    (cv::Mat_<double>(3, 3) << 0.40374166684570406138, 0.90997294703586928399,
     0.09456163130735180389, 0.90115147376238702304, -0.41339049372456904141,
     0.13051559689950803511, 0.15785654179833771837, 0.03251976877242616915,
     -0.98692642929980223254);
/** @brief Camera center in world coordinates for image P1000760. */
const cv::Mat Cw_760 = (cv::Mat_<double>(3, 1) << 34.71806465438230304699,
                        15.05396904925869350222, 108.78309929426337987479);
/** @brief Translation vector for image P1000760 (computed as -R * Cw). */
const cv::Mat T_760 = -R_760 * Cw_760;

// Image P1000761
/** @brief Rotation matrix for image P1000761 (world to camera). */
const cv::Mat R_761 =
    (cv::Mat_<double>(3, 3) << 0.39048684713889253439, 0.91135871022866510316,
     0.13017420444109242816, 0.90019452378457598396, -0.40760378423287696448,
     0.15332636572781230266, 0.19279481725344108090, 0.05731017683956705910,
     -0.97956403673819603117);
/** @brief Camera center in world coordinates for image P1000761. */
const cv::Mat Cw_761 = (cv::Mat_<double>(3, 1) << 24.25716505670521527804,
                        -5.50711477190438092322, 108.53368620854732284897);
/** @brief Translation vector for image P1000761 (computed as -R * Cw). */
const cv::Mat T_761 = -R_761 * Cw_761;

// Image P1000762
/** @brief Rotation matrix for image P1000762 (world to camera). */
const cv::Mat R_762 =
    (cv::Mat_<double>(3, 3) << 0.38829827695860857917, 0.91888814454634470952,
     0.06977840585132079332, 0.90984864426203304610, -0.39429400969095440566,
     0.12925818525869092745, 0.14628702145274621871, 0.01329705734384384330,
     -0.98915281712204528031);
/** @brief Camera center in world coordinates for image P1000762. */
const cv::Mat Cw_762 = (cv::Mat_<double>(3, 1) << 15.44836071864743765047,
                        -22.08237741087416239338, 107.98524305565048564404);
/** @brief Translation vector for image P1000762 (computed as -R * Cw). */
const cv::Mat T_762 = -R_762 * Cw_762;

/** @brief Stereo extrinsics for pair 1 (left: P1000760, right: P1000759). */
const StereoExtrinsics STEREO_EXTRINSICS_1{R_760, T_760, R_759, T_759};
/** @brief Stereo extrinsics for pair 2 (left: P1000761, right: P1000760). */
const StereoExtrinsics STEREO_EXTRINSICS_2{R_761, T_761, R_760, T_760};
/** @brief Stereo extrinsics for pair 3 (left: P1000762, right: P1000761). */
const StereoExtrinsics STEREO_EXTRINSICS_3{R_762, T_762, R_761, T_761};
