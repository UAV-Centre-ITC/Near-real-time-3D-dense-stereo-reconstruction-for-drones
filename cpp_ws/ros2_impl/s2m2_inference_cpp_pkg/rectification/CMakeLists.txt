cmake_minimum_required(VERSION 3.10)
project(StereoRectification)

# 1. Set C++ Standard
# C++17 is required for std::filesystem
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
# 2. Find Packages
# OpenCV
find_package(OpenCV REQUIRED)

set(CPP_SOURCES
src/stereo_rectifier.cpp
) 
# Define the executable
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(StereoRectifier ${CPP_SOURCES}) 

# 1. Include your local 'include/' folder
target_include_directories(StereoRectifier PUBLIC include/)
#Include also the OpenCV include directories: 
target_include_directories(StereoRectifier PRIVATE ${OpenCV_INCLUDE_DIRS})

# 2. Link OpenCV 
# This automatically handles include paths and library paths for OpenCV
target_link_libraries(StereoRectifier PRIVATE ${OpenCV_LIBS})
