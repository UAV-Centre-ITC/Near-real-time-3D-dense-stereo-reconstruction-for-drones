#include "stereo_rectifier.hpp"
#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

void drawEpipolarLines(const cv::Mat &img_left, const cv::Mat &img_right,
                       int num_lines);

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: test_sample_rectification <image_left filepath> "
                 "<image_right filepath>"
              << std::endl;
    return -1;
  }
  cv::Mat img_left;
  cv::Mat img_right;
  try {
    img_left = cv::imread(argv[1], cv::IMREAD_COLOR);
    img_right = cv::imread(argv[2], cv::IMREAD_COLOR);
  } catch (const cv::Exception &e) {
    std::cerr << "Error reading images: " << e.what() << std::endl;
    return -1;
  }
  // convert to RGB for the model input :
  cv::cvtColor(img_left, img_left, cv::COLOR_BGR2RGB);
  cv::cvtColor(img_right, img_right, cv::COLOR_BGR2RGB);
  /* Example intrinsic parameters for testing
   */
  // Samples 1000852 and 1000853 from the dataset of Francesco
  //
  // fileName imageWidth imageHeight
  // camera matrix K [3x3]
  // radial distortion [3x1]
  // tangential distortion [2x1]
  // camera position t [3x1]
  // camera rotation R [3x3]
  // camera model m = K [R|-Rt] X
  //
  //
  // P1000852_1400496954965.JPG 4592 3448
  //
  // 3837.04710692373510028119 0 2287.77960528229596093297
  // 0 3837.04710692373510028119 1742.83970496531173921539
  // 0 0 1
  // -0.04930618424089858870 0.04239222797251372687 -0.02928533838596112174
  // 0.00028446432839522286 -0.00085023797648524325
  // 53.81054947407682931271 2.73450993122391761148 109.15739203396617540420
  // -0.30242406816031625061 -0.95203343934820727767 0.04660486412583432492
  // -0.95061868820348527365 0.30483019730717991758 0.05833232764043053825
  // -0.06974089643565797858 -0.02666235496892283355 -0.99720876760679444395
  //
  // ===========================================================================
  // P1000853_1400496954965.JPG 4592 3448
  //
  // 3837.04710692373510028119 0 2287.77960528229596093297
  // 0 3837.04710692373510028119 1742.83970496531173921539
  // 0 0 1
  // -0.04930618424089858870 0.04239222797251372687 -0.02928533838596112174
  // 0.00028446432839522286 -0.00085023797648524325
  // 63.77929108604634222957 21.54632894130132925170 109.22231512122060337333
  // -0.30039254188295805292 -0.94706188642182642656 0.11330535763249140191
  // -0.94827780856518950614 0.30931607093561647170 0.07136361848837562138
  // -0.10263293118632564604 -0.08600785747874520326 -0.99099401102530326746
  cv::Mat K2 = (cv::Mat_<double>(3, 3) << 3837.0471, 0, 2287.7796, 0, 3837.0471,
                1742.8397, 0, 0, 1);
  cv::Mat K1 = (cv::Mat_<double>(3, 3) << 3837.0471, 0, 2287.7796, 0, 3837.0471,
                1742.8397, 0, 0, 1);
  // Fix the order of the D coefficients to match OpenCV's expected format: [k1,
  // k2, p1, p2, k3]
  cv::Mat D2 =
      (cv::Mat_<double>(1, 5) << -0.0493, 0.0424, 0.00028, -0.00085, -0.0292);
  cv::Mat D1 =
      (cv::Mat_<double>(1, 5) << -0.0493, 0.0424, 0.00028, -0.00085, -0.0292);
  const cv::Size img_size{4592, 3448};
  // cv::Mat K1 = (cv::Mat_<double>(3, 3) << 4874.4367, 0, 2999.3189, 0,
  // 4874.4370,
  //               1991.331, 0, 0, 1);
  // cv::Mat K2 = (cv::Mat_<double>(3, 3) << 4874.4367, 0, 2999.3189, 0,
  // 4874.4370,
  //               1991.331, 0, 0, 1);
  // cv::Mat D1 =
  //     (cv::Mat_<double>(1, 5) << -0.0196, -0.0202, -0.00047, 0.00044,
  //     0.1241);
  // cv::Mat D2 =
  //     (cv::Mat_<double>(1, 5) << -0.0196, -0.0202, -0.00047, 0.00044,
  //     0.1241);
  // const cv::Size img_size{6000, 4000};
  /* Example stereo intrinsics for testing
   */
  /*--------------------------------*/
  /* Example stereo extrinsics for testing
   */
  // R :
  //  -0.30242406816031625061 -0.95203343934820727767 0.04660486412583432492
  //  -0.95061868820348527365 0.30483019730717991758 0.05833232764043053825
  //  -0.06974089643565797858 -0.02666235496892283355 -0.99720876760679444395
  cv::Mat R2_world2cam = (cv::Mat_<double>(3, 3) << -0.3024, -0.9520, 0.0466,
                          -0.9506, 0.3048, 0.0583, -0.0697, -0.0267, -0.9972);
  cv::Mat Cw_2 = (cv::Mat_<double>(3, 1) << 53.8105, 2.7345, 109.1574);
  cv::Mat R1_world2cam = (cv::Mat_<double>(3, 3) << -0.3004, -0.9471, 0.1133,
                          -0.9482, 0.3093, 0.0714, -0.1026, -0.0860, -0.9910);
  cv::Mat Cw_1 = (cv::Mat_<double>(3, 1) << 63.7793, 21.5463, 109.2223);
  // cv::Mat R2_world2cam = (cv::Mat_<double>(3, 3) << 0.9479, -0.3173, -0.0278,
  //                          -0.3155, -0.9236, -0.2179, 0.0434, 0.2153,
  //                          -0.9756);
  //  cv::Mat Cw_2 = (cv::Mat_<double>(3, 1) << -12.236, 45.539, 115.994);

  // Calculate the translation vectors from world to camera,
  // T = -R * Cw
  // Transpose R1 , R2 because they are photogrammetry style (world to camera)
  R1_world2cam = R1_world2cam;
  R2_world2cam = R2_world2cam;
  cv::Mat T1_world2cam = -R1_world2cam * Cw_1;
  cv::Mat T2_world2cam = -R2_world2cam * Cw_2;
  /* Example stereo extrinsics for testing
   */

  const StereoIntrinsics stereo_intrinsics{K1, K2, D1, D2, img_size};
  const StereoExtrinsics stereo_extrinsics{R1_world2cam, T1_world2cam,
                                           R2_world2cam, T2_world2cam};
  /* Example stereo calibration data for testing
   */
  /*--------------------------------*/
  StereoRectifier rectifier(stereo_intrinsics);
  rectifier.rectify(stereo_extrinsics);
  cv::Mat rectified_left{rectifier.rectifyLeft(img_left)};
  cv::Mat rectified_right{rectifier.rectifyRight(img_right)};
  drawEpipolarLines(rectified_left, rectified_right, 20);
}

void drawEpipolarLines(const cv::Mat &img_left, const cv::Mat &img_right,
                       int num_lines = 10) {
  cv::Mat canvas;
  cv::hconcat(img_left, img_right, canvas);
  int step = img_left.rows / num_lines;
  for (int i = 0; i < num_lines; ++i) {
    int y = i * step;
    // increase the line thickness for better visibility

    cv::line(canvas, cv::Point(0, y), cv::Point(canvas.cols, y),
             cv::Scalar(0, 255, 0), 3);
  }
  // save the output image :
  cv::imwrite("epipolar_lines.png", canvas);
  // show the image in BGR format :
  cv::Mat display;
  cv::cvtColor(canvas, display, cv::COLOR_RGB2BGR);
  cv::imshow("Rectification Check", display);
  cv::waitKey(0);
}
