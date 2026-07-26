cmake_minimum_required(VERSION 3.10)
project(pfm2png)

find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${PNG_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

set(IMAGELIB_SOURCES
    include/imageLib/Convert.cpp
    include/imageLib/Convolve.cpp
    include/imageLib/Image.cpp
    include/imageLib/ImageIO.cpp
    include/imageLib/ImageIOpng.cpp
    include/imageLib/RefCntMem.cpp
)

add_executable(pfm2png
    pfm2png.cpp
    ${IMAGELIB_SOURCES}
)

target_link_libraries(pfm2png
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)
