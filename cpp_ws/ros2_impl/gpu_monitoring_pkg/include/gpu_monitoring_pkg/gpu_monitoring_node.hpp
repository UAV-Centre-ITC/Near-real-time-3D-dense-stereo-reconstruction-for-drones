#include "my_custom_interfaces/msg/gpu_metrics.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <nvml.h>
#include <rclcpp/publisher.hpp>
#include <rclcpp/rclcpp.hpp>

class GPUMonitoringNode : public rclcpp::Node {

public:
  GPUMonitoringNode();
  ~GPUMonitoringNode();
  void pollMetrics();

private:
  nvmlDevice_t device_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
  int poll_period_ms_ = 100;
  int count_polls_ = 0;
  int window_averaging_size_;
  // Memory & power :
  float mem_sum_ = 0;
  float power_sum_ = 0;
  unsigned long long min_mem_ = 1000000000;
  unsigned long long max_mem_ = 0;
  unsigned long long min_power_ = 100000000;
  unsigned long long max_power_ = 0;
  // PCIe :
  unsigned int pcie_generation_;
  unsigned int pcie_width_;
  unsigned long long max_pcie_bw_;
  float pcie_tx_bw_percentage_;
  float pcie_rx_bw_percentage_;
  rclcpp::Publisher<my_custom_interfaces::msg::GpuMetrics>::SharedPtr
      metrics_pub_;
  // private methods:
  unsigned long long calculateMaxPcieBwKBps(unsigned int gen,
                                            unsigned int width);
};
