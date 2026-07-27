#include <cuda_runtime_api.h>
#include <opencv2/core/cuda.hpp>
#include <s2m2_inference_cpp_pkg/cuda_kernels.h>

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

__constant__ float cq[16]; // Q matrix in constant memory

__global__ void reprojectTo3D(const cv::cuda::PtrStepSz<float> disp,
                               cv::cuda::PtrStepSz<float4> points3d) {
  const int x = blockIdx.x * blockDim.x + threadIdx.x;
  const int y = blockIdx.y * blockDim.y + threadIdx.y;

  if (y >= disp.rows || x >= disp.cols)
    return;

  const float qx = x * cq[0] + y * cq[1] + cq[3];
  const float qy = x * cq[4] + y * cq[5] + cq[7];
  const float qz = x * cq[8] + y * cq[9] + cq[11];
  const float qw = x * cq[12] + y * cq[13] + cq[15];

  // now request the disparity from global memory and create the final xyz
  // point:
  const float d = disp(y, x);
  const float iW = 1.f / (qw + cq[14] * d);

  float4 vec;
  vec = {(qx + cq[2] * d) * iW, (qy + cq[6] * d) * iW, (qz + cq[10] * d) * iW,
         1.f};

  points3d(y, x) = vec;
}

void launchReprojectionCustomKernel(const cv::cuda::PtrStepSz<float> disp,
                                    cv::cuda::PtrStepSz<float4> points3d,
                                    const float *Q, cudaStream_t stream) {
  dim3 blockDim(16, 16);
  dim3 grid((disp.cols + blockDim.x - 1) / blockDim.x,
            (disp.rows + blockDim.y - 1) / blockDim.y);
  CUDA_CHECK(cudaMemcpyToSymbolAsync(cq, Q, sizeof(float) * 16, 0,
                                     cudaMemcpyHostToDevice,
                                     stream)); // copy Q to constant mem
  reprojectTo3D<<<grid, blockDim, 0, stream>>>(disp, points3d);
  CUDA_CHECK(cudaGetLastError());
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
