cmake_minimum_required(VERSION 3.8)
project(dsm_generator_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set (CMAKE_BUILD_TYPE Release)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(dsm_node src/dsm_node.cpp)
target_include_directories(dsm_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)

# target_include_directories(dsm_node PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/../s2m2_inference_cpp_pkg/include
#   ${CMAKE_CURRENT_SOURCE_DIR}/../s2m2_inference_cpp_pkg/rectification/include
# )

  ament_target_dependencies(dsm_node
    rclcpp
    sensor_msgs
    geometry_msgs
    cv_bridge
    image_transport
  )

  target_link_libraries(dsm_node
    ${OpenCV_LIBS}
  )

  
install(TARGETS dsm_node
	DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
	DESTINATION share/${PROJECT_NAME})

install(DIRECTORY config
	DESTINATION share/${PROJECT_NAME})


ament_package()


