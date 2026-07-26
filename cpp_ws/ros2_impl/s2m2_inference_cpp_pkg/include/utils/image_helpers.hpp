#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>

/**
 * @brief Visualizes disparity, confidence, and occlusion in an OpenCV window.
 * @param height Image height.
 * @param width Image width.
 * @param disparity Disparity map (CV_32FC1).
 * @param confidence_out Confidence values vector.
 * @param occlusion_out Occlusion values vector.
 */
void visualize_output_disparity(int height, int width, cv::Mat &disparity,
                                std::vector<float> &confidence_out,
                                std::vector<float> &occlusion_out);

/**
 * @brief Saves disparity map as a color-mapped PNG, along with a masked version.
 * @param disparity Disparity map (CV_32FC1).
 * @param occlusion Occlusion map (CV_32FC1).
 * @param confidence Confidence map (CV_32FC1).
 * @param filename Output file path for the saved PNG.
 */
void saveDisparityToPng(cv::Mat &disparity, cv::Mat &occlusion,
                        cv::Mat &confidence, const std::string &filename);

/**
 * @brief Saves rectified images with horizontal green epipolar lines overlaid.
 * @param left_rectified Rectified left image.
 * @param right_rectified Rectified right image.
 * @param left_path Output file path for the left image.
 * @param right_path Output file path for the right image.
 */
void saveRectifiedImages(cv::Mat &left_rectified, cv::Mat &right_rectified,
                         const std::string &left_path,
                         const std::string &right_path);
