#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>

void visualize_output_disparity(int height, int width, cv::Mat &disparity,
                                std::vector<float> &confidence_out,
                                std::vector<float> &occlusion_out);

void saveDisparityToPfm(std::vector<float> &disparity_buffer, int height,
                        int width, const std::string &filename);

void saveRectifiedImages(cv::Mat &left_rectified, cv::Mat &right_rectified,
                         const std::string &left_path,
                         const std::string &right_path);
void saveOriginalImages(cv::Mat &left_image, cv::Mat &right_image,
                        const std::string &left_path,
                        const std::string &right_path);
