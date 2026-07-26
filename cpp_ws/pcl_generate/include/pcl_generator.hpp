#include <fstream>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <pcl/io/ply_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <string>

class PCLGenerator {
public:
  PCLGenerator() = default;
  std::optional<cv::Mat> loadPFM(const std::string &filename);
  std::optional<cv::Mat> readQMatrix(const std::string &filename);
  void generatePointCloud(const std::string &disparity_filename,
                          const std::string &q_matrix_filename);
  void savePointCloud(const std::string &filename,
                      const std::string &rectified_left_scaled_filename);

private:
  cv::Mat points3D_;
};
