#include <cuda_runtime_api.h>
#include <opencv2/core/cuda.hpp>
#include <s2m2_inference_cpp_pkg/cuda_kernels.h>
#include <stdio.h>

__global__ void transformPoint3D(TransformMatrix T,
                                 cv::cuda::PtrStepSz<float4> src_depthmap,
                                 cv::cuda::PtrStepSz<float4> dst_depthmap,
                                 int rows, int columns) {
  int col = blockIdx.x * blockDim.x + threadIdx.x;
  int row = blockIdx.y * blockDim.y + threadIdx.y;

  if (row < rows && col < columns) {
    float4 point = src_depthmap(row, col);
    // Apply the transform to the appropriate point in homogeneous coordinates
    float4 transformed_point_homogeneous{
        T.data[0] * point.x + T.data[1] * point.y + T.data[2] * point.z +
            T.data[3],
        T.data[4] * point.x + T.data[5] * point.y + T.data[6] * point.z +
            T.data[7],
        T.data[8] * point.x + T.data[9] * point.y + T.data[10] * point.z +
            T.data[11],
        1};
    dst_depthmap(row, col) = transformed_point_homogeneous;
  }
}

void launchTransformKernel(TransformMatrix T,
                           cv::cuda::PtrStepSz<float4> src_depthmap,
                           cv::cuda::PtrStepSz<float4> dst_depthmap, int rows,
                           int columns, cudaStream_t stream) {
  dim3 blockDim(16, 16);
  dim3 grid((columns + blockDim.x - 1) / blockDim.x,
            (rows + blockDim.y - 1) / blockDim.y);
  transformPoint3D<<<grid, blockDim, 0, stream>>>(T, src_depthmap, dst_depthmap,
                                                  rows, columns);
  CUDA_CHECK(cudaGetLastError());
}
// dim3 blockDim(16 , 16)
// dim3 grid((image_cols + blockDim.x - 1) / blockDim.x,
//           (image_rows + blockDim.y - 1) / blockDim.y);
