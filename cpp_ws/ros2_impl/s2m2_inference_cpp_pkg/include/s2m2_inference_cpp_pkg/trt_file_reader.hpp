#include <NvInfer.h>
#include <NvInferRuntime.h>
#include <fstream>

class FileStreamReader : public nvinfer1::v_1_0::IStreamReader {
public:
  explicit FileStreamReader(const std::string &path)
      : file_(path, std::ios::binary) {}

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
