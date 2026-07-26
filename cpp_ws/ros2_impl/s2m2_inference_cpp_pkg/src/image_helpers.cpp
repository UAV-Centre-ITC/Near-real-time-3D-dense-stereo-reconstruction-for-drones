#include "utils/image_helpers.hpp"
#include <cmath>
#include <limits>
#define DEBUG_MODE true

void visualize_output_disparity(int height, int width, cv::Mat &disparity,
                                std::vector<float> &confidence_out,
                                std::vector<float> &occlusion_out) {

  if (height <= 0 || width <= 0) {
    std::cout << "Invalid image size" << std::endl;
    return;
  }
  cv::Mat confidence(height, width, CV_32FC1, confidence_out.data());
  cv::Mat occlusion(height, width, CV_32FC1, occlusion_out.data());

  cv::Mat valid_mask = (occlusion > 0.5f) & (confidence > 0.1f);
  cv::Mat disparity_normalized;
  cv::normalize(disparity, disparity_normalized, 0, 255, cv::NORM_MINMAX,
                CV_8UC1);
  cv::applyColorMap(disparity_normalized, disparity_normalized,
                    cv::COLORMAP_JET);
  cv::imshow("Disparity map", disparity_normalized);

  cv::Mat disparity_masked;
  disparity.copyTo(disparity_masked, valid_mask);
  cv::imshow("Masked disparity", disparity_masked);
  cv::waitKey(0);
  return;
}

void saveDisparityToPng(cv::Mat &disparity, cv::Mat &occlusion,
                        cv::Mat &confidence, const std::string &filename) {
  cv::Mat disparity_normalized;
  cv::normalize(disparity, disparity_normalized, 0, 255, cv::NORM_MINMAX,
                CV_8UC1);
  cv::applyColorMap(disparity_normalized, disparity_normalized,
                    cv::COLORMAP_JET);
  cv::Mat disparity_masked;
  cv::Mat valid_mask = (occlusion > 0.5f) & (confidence > 0.1f);
  disparity.copyTo(disparity_masked, valid_mask);
  cv::imwrite(filename, disparity_normalized);
  cv::imwrite("disp_masked.png", disparity_masked);
}

void saveRectifiedImages(cv::Mat &left_rectified, cv::Mat &right_rectified,
                         const std::string &left_path,
                         const std::string &right_path) {
  const int num_lines = 15;
  cv::Scalar line_color(0, 255, 0);

  cv::Mat left_with_lines = left_rectified.clone();
  cv::Mat right_with_lines = right_rectified.clone();

  for (int i = 1; i < num_lines; ++i) {
    int y = static_cast<int>(left_rectified.rows * i / num_lines);
    cv::line(left_with_lines, cv::Point(0, y),
             cv::Point(left_rectified.cols, y), line_color, 1.5);
    cv::line(right_with_lines, cv::Point(0, y),
             cv::Point(right_rectified.cols, y), line_color, 1.5);
  }

  cv::imwrite(left_path, left_with_lines);
  cv::imwrite(right_path, right_with_lines);
}
