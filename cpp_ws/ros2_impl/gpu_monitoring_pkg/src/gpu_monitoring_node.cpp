#include "gpu_monitoring_pkg/gpu_monitoring_node.hpp"

GPUMonitoringNode::GPUMonitoringNode() : Node("gpu_monitoring_node") {
  // ros parameters
  this->declare_parameter("poll_frequency_hz", 10);
  this->declare_parameter("window_averaging_size", 100);
  int poll_freq = this->get_parameter("poll_frequency_hz").as_int();
  poll_period_ms_ = 1000 / poll_freq;
  window_averaging_size_ =
      this->get_parameter("window_averaging_size").as_int();
  // Initialize NVML
  nvmlReturn_t result = nvmlInit();
  if (NVML_SUCCESS != result) {
    RCLCPP_ERROR(this->get_logger(), "Failed to initialize NVML: %s",
                 nvmlErrorString(result));
    return;
  }
  // Get the handle for GPU 0
  result = nvmlDeviceGetHandleByIndex(0, &device_);
  if (NVML_SUCCESS != result) {
    RCLCPP_ERROR(this->get_logger(), "Failed to get GPU handle");
    return;
  }
  nvmlDeviceGetMaxPcieLinkGeneration(device_, &pcie_generation_);
  nvmlDeviceGetMaxPcieLinkWidth(device_, &pcie_width_);
  max_pcie_bw_ = calculateMaxPcieBwKBps(pcie_generation_, pcie_width_);

  // Publisher :
  metrics_pub_ = this->create_publisher<my_custom_interfaces::msg::GpuMetrics>(
      "gpu_metrics", 10);
  poll_timer_ =
      this->create_wall_timer(std::chrono::milliseconds(poll_period_ms_),
                              std::bind(&GPUMonitoringNode::pollMetrics, this));
}

GPUMonitoringNode::~GPUMonitoringNode() { nvmlShutdown(); }

void GPUMonitoringNode::pollMetrics() {
  count_polls_++;
  // Power usage (mW), published as watts :
  unsigned int power_usage = 0;
  if (nvmlDeviceGetPowerUsage(device_, &power_usage) == NVML_SUCCESS) {
    min_power_ =
        std::min(min_power_, static_cast<unsigned long long>(power_usage));
    max_power_ =
        std::max(max_power_, static_cast<unsigned long long>(power_usage));
    power_sum_ += power_usage;
  } else {
    RCLCPP_ERROR(this->get_logger(), "Failed to get power usage");
    return;
  }
  // Memory usage (MB, published as GB)
  nvmlMemory_t memory_info;
  if (nvmlDeviceGetMemoryInfo(device_, &memory_info) == NVML_SUCCESS) {
    unsigned long long memory_used_mb = memory_info.used / (1024 * 1024);
    min_mem_ = std::min(min_mem_, memory_used_mb);
    max_mem_ = std::max(max_mem_, memory_used_mb);
    mem_sum_ += memory_used_mb;
  } else {
    RCLCPP_ERROR(this->get_logger(), "Failed to get memory usage");
    return;
  }

  unsigned int pcie_tx_kbps = 0;
  unsigned int pcie_rx_kbps = 0;
  nvmlDeviceGetPcieThroughput(device_, NVML_PCIE_UTIL_TX_BYTES, &pcie_tx_kbps);
  nvmlDeviceGetPcieThroughput(device_, NVML_PCIE_UTIL_RX_BYTES, &pcie_rx_kbps);
  // Calculate utilization percentages (0.0 to 100.0)
  assert(max_pcie_bw_ > 0);
  pcie_tx_bw_percentage_ =
      (static_cast<float>(pcie_tx_kbps) / static_cast<float>(max_pcie_bw_)) *
      100.0f;
  pcie_rx_bw_percentage_ =
      (static_cast<float>(pcie_rx_kbps) / static_cast<float>(max_pcie_bw_)) *
      100.0f;

  // Publish metrics:
  my_custom_interfaces::msg::GpuMetrics metrics_msg;
  metrics_msg.stamp = this->now();
  metrics_msg.pcie_tx_bw_percentage = pcie_tx_bw_percentage_;
  metrics_msg.pcie_rx_bw_percentage = pcie_rx_bw_percentage_;
  metrics_msg.min_memory_mb = min_mem_;
  metrics_msg.max_memory_mb = max_mem_;
  metrics_msg.avg_memory_mb = static_cast<float>(mem_sum_) / static_cast<float>(count_polls_);
  metrics_msg.min_power_w = static_cast<float>(min_power_) / 1000.0f;
  metrics_msg.max_power_w = static_cast<float>(max_power_) / 1000.0f;
  metrics_msg.avg_power_w = static_cast<float>(power_sum_) / static_cast<float>(count_polls_) / 1000.0f;
  metrics_pub_->publish(metrics_msg);
  // reset the averaging window
  if (count_polls_ >= window_averaging_size_) {
    mem_sum_ = 0;
    power_sum_ = 0;
    count_polls_ = 0;
  }
}

unsigned long long
GPUMonitoringNode::calculateMaxPcieBwKBps(unsigned int gen,
                                          unsigned int width) {
  // Approximate theoretical max bandwidth per lane in KB/s
  unsigned long long kbps_per_lane = 0;
  switch (gen) {
  case 1:
    kbps_per_lane = 250000;
    break; // ~250 MB/s
  case 2:
    kbps_per_lane = 500000;
    break; // ~500 MB/s
  case 3:
    kbps_per_lane = 985000;
    break; // ~985 MB/s
  case 4:
    kbps_per_lane = 1969000;
    break; // ~1969 MB/s
  case 5:
    kbps_per_lane = 3938000;
    break; // ~3938 MB/s
  default:
    kbps_per_lane = 985000;
    RCLCPP_WARN(this->get_logger(), "Unknown PCIe generation %d; using %llu",
                gen, kbps_per_lane);
    break;
  }
  return kbps_per_lane * width;
}

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<GPUMonitoringNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();

  return 0;
}
