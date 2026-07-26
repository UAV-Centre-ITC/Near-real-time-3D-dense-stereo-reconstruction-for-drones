#include "pcl_generator.hpp"
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <optional>
#include <pcl/impl/point_types.hpp>

std::optional<cv::Mat> PCLGenerator::loadPFM(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open PFM file: " << filename << std::endl;
    return std::nullopt;
  }

  std::string header;
  int width, height;
  float scale;

  // 1. Read Header (Type, Width, Height, Scale)
  file >> header; // Should be "Pf" for grayscale or "PF" for color
  file >> width >> height;
  file >> scale;

  // Skip the single newline character after the scale
  file.get();

  // 2. Prepare cv::Mat (32-bit float, 1 channel for disparity)
  cv::Mat img(height, width, CV_32FC1);

  // 3. Read raw data
  // PFM stores data from bottom to top, so we read it into the Mat
  file.read(reinterpret_cast<char *>(img.data), width * height * sizeof(float));

  if (file.gcount() != width * height * sizeof(float)) {
    throw std::runtime_error("Error reading pixel data from PFM.");
  }

  // 4. Handle Orientation & Endianness
  // Since your Python code used np.flipud, we flip it back
  cv::flip(img, img, 0);

  // PFM Convention: scale < 0 means Little Endian (Standard for x86)
  // If scale > 0, the file is Big Endian and would need byte swapping.
  if (scale > 0) {
    std::cerr
        << "Warning: Big Endian PFM detected. Data may need byte swapping."
        << std::endl;
  }

  return img;
}

std::optional<cv::Mat> PCLGenerator::readQMatrix(const std::string &filename) {
  cv::Mat Q_loaded;

  // 1. Open FileStorage for reading
  cv::FileStorage fs(filename, cv::FileStorage::READ);

  // 2. Check if file exists
  if (!fs.isOpened()) {
    std::cerr << "Failed to open file!" << std::endl;
    return std::nullopt;
  }

  // 3. Use the key "Q_matrix" to pull data into Q_loaded
  fs["Q"] >> Q_loaded;

  // 4. Close the file
  fs.release();

  // Verify the results
  std::cout << "Loaded Q Matrix: \n" << Q_loaded << std::endl;
  return Q_loaded;
}

void PCLGenerator::generatePointCloud(const std::string &disparity_file_,
                                      const std::string &q_matrix_file_) {
  // 1. Load Disparity Map
  auto disparity_opt = loadPFM(disparity_file_);
  if (disparity_opt.has_value() == false) {
    std::cerr << "Failed to load disparity map." << std::endl;
    return;
  }
  cv::Mat disparity = disparity_opt.value();

  // 2. Load Q Matrix
  auto Q_opt = readQMatrix(q_matrix_file_);
  if (!Q_opt) {
    std::cerr << "Failed to load Q matrix." << std::endl;
    return;
  }
  cv::Mat Q = Q_opt.value();

  // 3. Reproject to 3D
  cv::reprojectImageTo3D(disparity, points3D_, Q, true);

  // For demonstration, print the shape of the generated point map
  std::cout << "Generated Points3D shape : " << points3D_.size() << std::endl;
}

// Use PCL library to save the point cloud to a .ply file
void PCLGenerator::savePointCloud(
    const std::string &filename,
    const std::string &rectified_left_scaled_filename) {

  // 1. Initialize the PointCloud
  // Using PointXYZ. If you have color, use PointXYZRGB
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(
      new pcl::PointCloud<pcl::PointXYZRGB>);
  // TODO optimize this by pre-allocating the cloud size based on points3D_
  // dimensions
  //
  //
  //  Read the rectified left image to get color information (if needed)
  cv::Mat rectified_left =
      cv::imread(rectified_left_scaled_filename, cv::IMREAD_COLOR);
  //  2. Set dimensions for an organized cloud
  cloud->width = points3D_.cols;
  cloud->height = points3D_.rows;
  cloud->is_dense = false; // Set to false because we likely have 'NaN' points
  cloud->points.resize(cloud->width * cloud->height);

  // 3. Iterate through the cv::Mat
  for (int v = 0; v < points3D_.rows; ++v) {
    for (int u = 0; u < points3D_.cols; ++u) {
      // Get the (X, Y, Z) vector from the matrix
      cv::Vec3f point = points3D_.at<cv::Vec3f>(v, u);

      pcl::PointXYZRGB &pcl_point = cloud->at(u, v);

      // 4. Handle invalid points (OpenCV often uses a large constant like
      // 10000.0) You should check your specific 'missing value' threshold
      if (std::isinf(point[2]) || point[2] > 10000.0f || point[2] <= 0.0f) {
        pcl_point.x = pcl_point.y = pcl_point.z =
            std::numeric_limits<float>::quiet_NaN();
      } else {
        pcl_point.x = point[0];
        pcl_point.y = point[1];
        pcl_point.z = point[2];

        // Assign color from the rectified left image (if available)
        if (!rectified_left.empty()) {
          cv::Vec3b color = rectified_left.at<cv::Vec3b>(v, u);
          pcl_point.r = color[2]; // OpenCV uses BGR format
          pcl_point.g = color[1];
          pcl_point.b = color[0];
        } else {
          pcl_point.r = pcl_point.g = pcl_point.b = 255; // Default to white
        }
      }
    }
  }
  // 5. Save the point cloud to a .ply file
  if (pcl::io::savePLYFileBinary(filename, *cloud) == -1) {
    std::cerr << "Failed to save point cloud to " << filename << std::endl;
  } else {
    std::cout << "Point cloud saved to " << filename << std::endl;
  }
}

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <disparity_file.pfm> <q_matrix_file.yml> "
                 "<rectified_left_scaled filename>"
              << std::endl;
    return -1;
  }

  std::string disparity_file = argv[1];
  std::string q_matrix_file = argv[2];
  std::string rectified_left_scaled_filename = argv[3];
  PCLGenerator generator;
  generator.generatePointCloud(disparity_file, q_matrix_file);
  generator.savePointCloud("output_point_cloud.ply",
                           rectified_left_scaled_filename);

  return 0;
}
