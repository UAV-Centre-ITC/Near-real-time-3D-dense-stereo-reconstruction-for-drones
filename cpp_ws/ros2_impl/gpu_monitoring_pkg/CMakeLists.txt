cmake_minimum_required(VERSION 3.8)
project(gpu_monitoring_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(my_custom_interfaces REQUIRED)

find_library(NVML_LIBRARY 
  NAMES nvidia-ml libnvidia-ml.so.1 nvidia-ml.so.1
  PATHS /usr/lib/x86_64-linux-gnu/ /usr/local/cuda/lib64/
  NO_DEFAULT_PATH # Optional: forces it to use your paths first
)

# 2. It's good practice to also find the headers
find_path(NVML_INCLUDE_DIR
  NAMES nvml.h
  PATHS /usr/local/cuda/include /usr/include
)

# 3. Validation: Stop the build early with a clear message if not found
if(NOT NVML_LIBRARY)
  message(FATAL_ERROR "NVML library not found! Please check if NVIDIA drivers/CUDA are installed.")
endif()
add_executable(gpu_monitoring_node src/gpu_monitoring_node.cpp)

target_include_directories(gpu_monitoring_node PUBLIC
  ${NVML_INCLUDE_DIR}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_compile_features(gpu_monitoring_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17

ament_target_dependencies(gpu_monitoring_node rclcpp my_custom_interfaces)

target_link_libraries(gpu_monitoring_node ${NVML_LIBRARY})

install(TARGETS gpu_monitoring_node
  DESTINATION lib/${PROJECT_NAME})


ament_package()

