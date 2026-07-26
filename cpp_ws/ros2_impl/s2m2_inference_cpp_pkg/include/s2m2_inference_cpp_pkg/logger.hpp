#include <NvInfer.h>
#include <iostream>

enum class Severity {
  kINTERNAL_ERROR = 0,
  kERROR = 1,
  kWARNING = 2,
  kINFO = 3,
  kVERBOSE = 4
};

class TensorRTLogger : public nvinfer1::ILogger {
  void log(Severity severity, const char *msg) noexcept override {
    // Filter by severity level
    if (severity <= Severity::kWARNING) {
      std::cout << "[TensorRT] " << msg << std::endl;
    }
  }
};
