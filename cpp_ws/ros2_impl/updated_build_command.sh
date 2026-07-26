colcon build --symlink-install \
  --allow-overriding cv_bridge image_geometry my_custom_interfaces \
  --packages-skip opencv_tests \
  --cmake-args -DOpenCV_DIR=/opt/opencv_cuda/lib/cmake/opencv4 \
  -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-13 \
  -DCMAKE_CUDA_COMPILER=/usr/local/cuda-13/bin/nvcc
