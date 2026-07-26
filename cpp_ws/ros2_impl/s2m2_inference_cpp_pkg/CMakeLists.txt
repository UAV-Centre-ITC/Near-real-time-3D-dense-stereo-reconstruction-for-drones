cmake_minimum_required(VERSION 3.8)
project(s2m2_inference_cpp_pkg LANGUAGES CXX CUDA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_BUILD_TYPE Release)
set(OpenCV_DIR "/opt/opencv_cuda/lib/cmake/opencv4")

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(message_filters REQUIRED)
find_package(image_transport REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(Eigen3 REQUIRED)

# fallback search if TensorRTConfig.cmake is missing
find_path(TRT_INCLUDE_DIR NvInfer.h
    HINTS /usr/include/x86_64-linux-gnu /usr/local/cuda/include
)

find_library(TRT_LIB nvinfer
    HINTS /usr/lib/x86_64-linux-gnu /usr/local/cuda/lib64
)

if(TRT_INCLUDE_DIR AND TRT_LIB)
    message(STATUS "Found TensorRT headers: ${TRT_INCLUDE_DIR}")
    message(STATUS "Found TensorRT library: ${TRT_LIB}")
    
    # Create an imported target to make it act like find_package
    add_library(TensorRT::TensorRT UNKNOWN IMPORTED)
    set_target_properties(TensorRT::TensorRT PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TRT_INCLUDE_DIR}"
        IMPORTED_LOCATION "${TRT_LIB}"
    )
else()
    message(FATAL_ERROR "TensorRT not found. Please check your installation.")
endif()

set(SOURCE_FILES 
	src/s2m2_node.cpp 
	src/image_helpers.cpp
	src/cuda_kernels.cu
)

include_directories(
  include
)
add_subdirectory(rectification)

add_executable(s2m2_node ${SOURCE_FILES})

target_include_directories(s2m2_node PUBLIC
  $<BUILD_INTERFACE:/usr/include/eigen3>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rectification/include>
  $<INSTALL_INTERFACE:include)
target_compile_features(s2m2_node PUBLIC cxx_std_17)  # Require C++17

target_link_libraries(s2m2_node
  StereoRectifier
  ${OpenCV_LIBS}
  CUDA::cudart
  TensorRT::TensorRT
  )

ament_target_dependencies(s2m2_node 
	rclcpp 
	sensor_msgs
	geometry_msgs
       	cv_bridge
	OpenCV
	message_filters
	image_transport
	Eigen3
	tf2_eigen
) 

install(TARGETS s2m2_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY 
config
launch
  DESTINATION share/${PROJECT_NAME}
)
  
ament_package()
