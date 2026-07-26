#include <NvInfer.h>
#include <NvInferRuntime.h>
#include <fstream>

/** @brief Reads a TensorRT engine file from disk via the IStreamReader interface. */
class FileStreamReader : public nvinfer1::v_1_0::IStreamReader {
public:
  /**
   * @brief Opens the engine file at the given path for binary reading.
   * @param path File path to the serialized TensorRT engine.
   */
  explicit FileStreamReader(const std::string &path)
      : file_(path, std::ios::binary) {}

  /**
   * @brief Reads up to @p size bytes from the file into @p dst.
   * @param dst Destination buffer.
   * @param size Number of bytes to read.
   * @return Number of bytes actually read.
   */
  int64_t read(void *dst, int64_t size) noexcept override {
    if (!file_.good())
      return 0;
    file_.read(static_cast<char *>(dst), size);
    return file_.gcount();
  }
  // This seems unnecessary in tensorrt 10.13.3, unless I use the
  // IStreamReaderV2 :
  // //
  // bool seek(int64_t offset) noexcept override {
  //   if (!file_.good())
  //     return false;
  //   file_.seekg(offset, std::ios::beg);
  //   return file_.good();
  // }
  //
private:
  std::ifstream file_;
};
