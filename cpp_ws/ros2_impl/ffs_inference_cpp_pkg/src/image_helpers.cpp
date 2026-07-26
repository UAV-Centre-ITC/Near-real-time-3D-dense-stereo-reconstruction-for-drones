#include "utils/image_helpers.hpp"

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

void saveDisparityToPfm(std::vector<float> &disparity_buffer, int height,
                        int width, const std::string &filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + filename);
  }
  // #if DEBUG_MODE
  //   float max = 0;
  //   float min = 100000;
  //   for (size_t i = 0; i < disparity_buffer.size(); i++) {
  //     if (disparity_buffer[i] > max) {
  //       max = disparity_buffer[i];
  //     }
  //     if (disparity_buffer[i] < min) {
  //       min = disparity_buffer[i];
  //     }
  //   }
  //   RCLCPP_INFO(this->get_logger(), "Max disparity value: %f", max);
  //   RCLCPP_INFO(this->get_logger(), "Min disparity value: %f", min);
  // #endif
  // Write PFM header (Middlebury format)
  file << "Pf\n";
  file << width << " " << height << "\n";
  file << -1.0f << "\n";

  // Write data in reverse vertical order (flipud)
  // PFM uses row-major order, bottom-to-top
  for (int row = height - 1; row >= 0; --row) {
    const float *rowPtr = disparity_buffer.data() + row * width;
    file.write(reinterpret_cast<const char *>(rowPtr), width * sizeof(float));
  }

  file.close();
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
             cv::Point(left_rectified.cols, y), line_color, 1);
    cv::line(right_with_lines, cv::Point(0, y),
             cv::Point(right_rectified.cols, y), line_color, 1);
  }

  cv::imwrite(left_path, left_with_lines);
  cv::imwrite(right_path, right_with_lines);
}
