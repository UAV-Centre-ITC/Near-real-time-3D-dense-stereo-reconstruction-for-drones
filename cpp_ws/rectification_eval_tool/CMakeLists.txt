cmake_minimum_required(VERSION 3.8) 
project(rectification_eval_tool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenCV REQUIRED)

add_executable(evaluate_rectification src/evaluate_rectification.cpp)

target_include_directories(evaluate_rectification PUBLIC 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>) 

target_link_libraries(evaluate_rectification ${OpenCV_LIBS})



