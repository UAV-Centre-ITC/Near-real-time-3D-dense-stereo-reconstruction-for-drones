cmake_minimum_required(VERSION 3.8)
project(pointcloud_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set (CMAKE_EXPORT_COMPILE_COMMANDS ON) #for neovim clangd
set (CMAKE_BUILD_TYPE Release)

#Packages needed : 
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(my_custom_interfaces REQUIRED)
find_package(PCL REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(message_filters REQUIRED)


add_library(pointcloud_generator_component SHARED src/pointcloud_generator.cpp)
target_include_directories(pointcloud_generator_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PCL_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_compile_features(pointcloud_generator_component PUBLIC c_std_99 cxx_std_17)


ament_target_dependencies(pointcloud_generator_component rclcpp rclcpp_components sensor_msgs cv_bridge my_custom_interfaces pcl_conversions message_filters)

target_link_libraries(pointcloud_generator_component 
	${OpenCV_LIBS}
	${PCL_LIBRARIES}
)

rclcpp_components_register_node(pointcloud_generator_component
  PLUGIN "pointcloud_pkg::PointcloudGenerator"
  EXECUTABLE pointcloud_generator
)

install(TARGETS pointcloud_generator_component
  DESTINATION lib)
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})
ament_package()
