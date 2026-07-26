#include "NvInfer.h"
#include <iostream>
#include <stdexcept>
#include <vector>
/**
 * @brief Manages a CUDA device memory buffer for TensorRT tensor I/O.
 * Supports move semantics; copy is disallowed.
 */
class CudaBuffer {
  void *d_ptr = nullptr;
  size_t size_ = 0;

public:
  /**
   * @brief Allocates a CUDA buffer of the given size.
   * @param size Number of bytes to allocate on the device.
   */
  CudaBuffer(size_t size) : size_(size) { cudaMalloc(&d_ptr, size); }
  /** @brief Frees the CUDA device memory. */
  ~CudaBuffer() { cudaFree(d_ptr); }
  CudaBuffer(const CudaBuffer &other) = delete;
  CudaBuffer &operator=(const CudaBuffer &other) = delete;
  /**
   * @brief Move constructor; transfers ownership of the device pointer.
   */
  CudaBuffer(CudaBuffer &&other) noexcept {
    this->d_ptr = other.d_ptr;
    this->size_ = other.size_;
    other.d_ptr = nullptr;
    other.size_ = 0;
  }
  /** @brief Returns the raw device pointer. */
  void *data() { return d_ptr; }
  /** @brief Returns the buffer size in bytes. */
  size_t size() const { return size_; }
};

/**
 * @brief Computes the total byte size of a TensorRT engine tensor from its name.
 * @param engine Pointer to the deserialized CUDA engine.
 * @param tensorName Name of the tensor to query.
 * @return Total size in bytes (dims volume * element byte width).
 */
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
