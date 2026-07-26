cmake_minimum_required(VERSION 3.10)
project(StereoCalibrationTool)

# 1. Set C++ Standard
# C++17 is required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 2. Find Packages
# OpenCV
find_package(OpenCV REQUIRED)

# Define the executable
add_executable(stereo_rectifier src/stereo_rectifier.cpp)

# 1. Include your local 'include/' folder
target_include_directories(stereo_rectifier PRIVATE include/)

# 2. Link OpenCV 
# This automatically handles include paths and library paths for OpenCV
target_link_libraries(stereo_rectifier PRIVATE ${OpenCV_LIBS})
