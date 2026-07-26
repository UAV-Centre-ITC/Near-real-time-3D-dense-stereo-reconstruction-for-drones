#include <NvInfer.h>
#include <iostream>

enum class Severity {
  kINTERNAL_ERROR = 0,
  kERROR = 1,
  kWARNING = 2,
  kINFO = 3,
  kVERBOSE = 4
};

/** @brief Custom TensorRT logger that prints messages with severity <= kWARNING. */
class TensorRTLogger : public nvinfer1::ILogger {
  /**
   * @brief Logs a TensorRT message if its severity is <= kWARNING.
   * @param severity Severity level of the log message.
   * @param msg The log message string.
   */
  void log(Severity severity, const char *msg) noexcept override {
    // Filter by severity level
    if (severity <= Severity::kWARNING) {
      std::cout << "[TensorRT] " << msg << std::endl;
    }
  }
};
