#include "opencv4/opencv2/core.hpp"
#include "opencv4/opencv2/features2d.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include <algorithm>
#include <iostream>
#include <opencv4/opencv2/core/cvstd.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <opencv4/opencv2/imgproc.hpp>

void evaluate_rectificaiton(cv::Mat &left_gray, cv::Mat &right_gray);
void calculate_statistics(std::vector<float> errors_vect);

void evaluate_rectificaiton(cv::Mat &left_gray, cv::Mat &right_gray) {
  // Create SIFT detector :
  cv::Ptr<cv::SIFT> sift_detector = cv::SIFT::create();

  // Detect keypoints in left and right images
  std::vector<cv::KeyPoint> keypoints_left;
  std::vector<cv::KeyPoint> keypoints_right;
  cv::Mat descriptors_left;
  cv::Mat descriptors_right;
  sift_detector->detectAndCompute(left_gray, cv::noArray(), keypoints_left,
                                  descriptors_left);
  sift_detector->detectAndCompute(right_gray, cv::noArray(), keypoints_right,
                                  descriptors_right);

  cv::Ptr<cv::DescriptorMatcher> matcher =
      cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);

  // Left -> Right matching
  std::vector<std::vector<cv::DMatch>> knn_matches_lr;
  std::vector<cv::DMatch> reliable_lr; // based on Lowe's ratio test
  matcher->knnMatch(descriptors_left, descriptors_right, knn_matches_lr, 2);
  // Lowe's ratio test :
  for (size_t i = 0; i < knn_matches_lr.size(); i++) {
    if (knn_matches_lr[i].size() == 2) {
      float distance1 = knn_matches_lr[i][0].distance;
      float distance2 = knn_matches_lr[i][1].distance;
      if (distance1 < distance2 * 0.7) {
        reliable_lr.push_back(
            knn_matches_lr[i][0]); // quite unique match, keep it
      }
    }
  }
  // Right -> Left matching
  std::vector<std::vector<cv::DMatch>> knn_matches_rl;
  std::vector<cv::DMatch> reliable_rl;
  matcher->knnMatch(descriptors_right, descriptors_left, knn_matches_rl, 2);
  // Lowe's ratio test :
  for (size_t i = 0; i < knn_matches_rl.size(); i++) {
    if (knn_matches_rl[i].size() == 2) {
      float d1 = knn_matches_rl[i][0].distance;
      float d2 = knn_matches_rl[i][1].distance;
      if (d1 < d2 * 0.7)
        reliable_rl.push_back(knn_matches_rl[i][0]);
    }
  }
  // We cross check the mathces (left->right and right->left) to keep only the
  // common ones
  std::vector<cv::DMatch> reliable_matches;
  for (size_t i = 0; i < reliable_lr.size(); i++) {
    bool found = false;
    for (size_t j = 0; j < reliable_rl.size(); j++) {
      if (reliable_lr[i].queryIdx == reliable_rl[j].trainIdx &&
          reliable_lr[i].trainIdx == reliable_rl[j].queryIdx) {
        found = true;
        break;
      }
    }
    if (found) {
      reliable_matches.push_back(reliable_lr[i]);
    }
  }

  std::vector<float> errors_vect;
  for (size_t i = 0; i < reliable_matches.size(); i++) {
    // Get the keypoints from the left and right images
    cv::KeyPoint left_keypoint = keypoints_left[reliable_matches[i].queryIdx];
    cv::KeyPoint right_keypoint = keypoints_right[reliable_matches[i].trainIdx];

    // Vertical distance calculation :
    float diff_Y = std::abs(right_keypoint.pt.y - left_keypoint.pt.y);
    errors_vect.push_back(diff_Y);
  }
  calculate_statistics(errors_vect);
}

void calculate_statistics(std::vector<float> errors_vect) {
  // Sort the errors :
  std::sort(errors_vect.begin(), errors_vect.end());
  // remove the highest 5% of the errors :
  size_t size = errors_vect.size();
  size_t for_removal = size / 20;

  for (size_t i = 0; i < for_removal; i++) {
    errors_vect.pop_back();
  }
  float average_pixel_error = 0.0;
  float maximum_pixel_error = 0.0;
  int count = 0;
  // Iterate over the matches
  for (size_t i = 0; i < errors_vect.size(); i++) {
    // Update the average and maximum pixel errors
    average_pixel_error += errors_vect[i];
    maximum_pixel_error = std::max(maximum_pixel_error, errors_vect[i]);
    count++;
  }
  average_pixel_error /= count;

  std::cout << "---------- RESULTS ----------" << std::endl;
  std::cout << "Average vertical pixel error: " << average_pixel_error
            << std::endl;
  std::cout << "Maximum vertical pixel error: " << maximum_pixel_error
            << std::endl;
  std::cout << "Keypoints used : " << count << std::endl;
  std::cout << "-----------------------------" << std::endl;
}

int main(int argc, char **argv) {
  // verify that 2 arguments are passed :

  if (argc != 3) {
    std::cout << "Usage : ./evaluate_rectification <left_rectified_path> "
                 "<right_rectified_path> "
              << std::endl;
    return -1;
  }

  std::string left_rectified_path = argv[1];
  std::string right_rectified_path = argv[2];

  cv::Mat left_rectified;
  cv::Mat right_rectified;

  try {
    left_rectified = cv::imread(left_rectified_path);
  } catch (cv::Exception &e) {
    std::cout << "Exception opening image " << left_rectified_path
              << ". Reason: " << e.what() << std::endl;
    return -1;
  }
  try {
    right_rectified = cv::imread(right_rectified_path);
  } catch (cv::Exception &e) {
    std::cout << "Exception opening image " << right_rectified_path
              << ". Reason: " << e.what() << std::endl;
    return -1;
  }
  cv::Mat left_gray;
  cv::Mat right_gray;
  cv::cvtColor(left_rectified, left_gray, cv::COLOR_BGR2GRAY);
  cv::cvtColor(right_rectified, right_gray, cv::COLOR_BGR2GRAY);
  evaluate_rectificaiton(left_gray, right_gray);
  return 0;
}
