cmake_minimum_required(VERSION 3.10)
project(pcl_generate)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)


set(PCL_VERBOSE ON)
find_package(PCL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)

set(CPP_SOURCES
src/pcl_generator.cpp
)

add_executable(pcl_generator ${CPP_SOURCES})

target_include_directories(pcl_generator PUBLIC
${PCL_INCLUDE_DIRS}
${OpenCV_INCLUDE_DIRS}
)

target_include_directories(pcl_generator PRIVATE
include
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(pcl_generator PUBLIC OpenMP::OpenMP_CXX)
endif()

target_link_libraries(pcl_generator PUBLIC
${PCL_LIBRARIES}
${OpenCV_LIBS}
)


