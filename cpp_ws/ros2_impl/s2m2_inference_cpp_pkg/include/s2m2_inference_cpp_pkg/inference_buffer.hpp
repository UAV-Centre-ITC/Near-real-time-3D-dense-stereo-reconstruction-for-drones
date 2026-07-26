#include "NvInfer.h"
#include <iostream>
#include <stdexcept>
#include <vector>
// stores the data,size, and dimensions of each cuda buffer (equivalent to the
// pytorch tensors) and manages the cuda memory
class CudaBuffer {
  void *d_ptr = nullptr;
  size_t size_ = 0;

public:
  CudaBuffer(size_t size) : size_(size) { cudaMalloc(&d_ptr, size); }
  ~CudaBuffer() { cudaFree(d_ptr); }
  CudaBuffer(const CudaBuffer &other) = delete;
  CudaBuffer &operator=(const CudaBuffer &other) = delete;
  CudaBuffer(CudaBuffer &&other) noexcept {
    this->d_ptr = other.d_ptr;
    this->size_ = other.size_;
    other.d_ptr = nullptr;
    other.size_ = 0;
  }
  void *data() { return d_ptr; }
  size_t size() const { return size_; }
};

// helper function to get the size of a tensor from the engine
size_t getSizeFromBinding(nvinfer1::ICudaEngine *engine,
                          const char *tensorName) {
  nvinfer1::Dims dims = engine->getTensorShape(tensorName);
  nvinfer1::DataType dtype = engine->getTensorDataType(tensorName);
  int elementCount = 1;
  for (int i = 0; i < dims.nbDims; i++) {
    elementCount *= dims.d[i];
  }
  int bytesPerElement = 0;
  std::cerr << "Tensor dtype value: " << static_cast<int>(dtype) << " for "
            << tensorName << std::endl;
  switch (dtype) {
  case nvinfer1::DataType::kHALF:
    bytesPerElement = 2; // f16 is not(?) defined type in cpp 17?
    break;
  case nvinfer1::DataType::kFLOAT:
    bytesPerElement = sizeof(float);
    break;
  case nvinfer1::DataType::kINT8:
    bytesPerElement = sizeof(char);
    break;
  case nvinfer1::DataType::kINT32:
    bytesPerElement = sizeof(int);
    break;
  case nvinfer1::DataType::kUINT8:
    bytesPerElement = sizeof(unsigned char);
    break;
  default:
    throw std::runtime_error("Unsupported data type");
  }
  return bytesPerElement * elementCount;
}
