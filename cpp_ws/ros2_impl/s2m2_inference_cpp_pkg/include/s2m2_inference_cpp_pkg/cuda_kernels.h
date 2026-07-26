#pragma once
#include <cuda_runtime_api.h>

#define CUDA_CHECK(expr_to_check)                                              \
  do {                                                                         \
    cudaError_t result = expr_to_check;                                        \
    if (result != cudaSuccess) {                                               \
      fprintf(stderr, "CUDA Runtime Error: %s:%i:%d = %s\n", __FILE__,         \
              __LINE__, result, cudaGetErrorString(result));                   \
    }                                                                          \
  } while (0)

struct TransformMatrix {
  float data[16];
};

void launchReprojectionCustomKernel(const cv::cuda::PtrStepSz<float> disp,
                                    cv::cuda::PtrStepSz<float4> points3d,
                                    const float *Q, const int offset_x,
                                    const int offset_y, cudaStream_t stream);

void launchTransformKernel(TransformMatrix T,
                           cv::cuda::PtrStepSz<float4> src_depthmap,
                           cv::cuda::PtrStepSz<float4> dst_depthmap, int rows,
                           int columns, cudaStream_t stream);
