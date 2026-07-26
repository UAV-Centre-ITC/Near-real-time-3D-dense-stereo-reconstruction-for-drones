// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/gpu_metrics.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__BUILDER_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "my_custom_interfaces/msg/detail/gpu_metrics__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace my_custom_interfaces
{

namespace msg
{

namespace builder
{

class Init_GpuMetrics_stamp
{
public:
  explicit Init_GpuMetrics_stamp(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  ::my_custom_interfaces::msg::GpuMetrics stamp(::my_custom_interfaces::msg::GpuMetrics::_stamp_type arg)
  {
    msg_.stamp = std::move(arg);
    return std::move(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_pcie_rx_bw_percentage
{
public:
  explicit Init_GpuMetrics_pcie_rx_bw_percentage(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_stamp pcie_rx_bw_percentage(::my_custom_interfaces::msg::GpuMetrics::_pcie_rx_bw_percentage_type arg)
  {
    msg_.pcie_rx_bw_percentage = std::move(arg);
    return Init_GpuMetrics_stamp(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_pcie_tx_bw_percentage
{
public:
  explicit Init_GpuMetrics_pcie_tx_bw_percentage(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_pcie_rx_bw_percentage pcie_tx_bw_percentage(::my_custom_interfaces::msg::GpuMetrics::_pcie_tx_bw_percentage_type arg)
  {
    msg_.pcie_tx_bw_percentage = std::move(arg);
    return Init_GpuMetrics_pcie_rx_bw_percentage(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_avg_power_w
{
public:
  explicit Init_GpuMetrics_avg_power_w(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_pcie_tx_bw_percentage avg_power_w(::my_custom_interfaces::msg::GpuMetrics::_avg_power_w_type arg)
  {
    msg_.avg_power_w = std::move(arg);
    return Init_GpuMetrics_pcie_tx_bw_percentage(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_max_power_w
{
public:
  explicit Init_GpuMetrics_max_power_w(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_avg_power_w max_power_w(::my_custom_interfaces::msg::GpuMetrics::_max_power_w_type arg)
  {
    msg_.max_power_w = std::move(arg);
    return Init_GpuMetrics_avg_power_w(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_min_power_w
{
public:
  explicit Init_GpuMetrics_min_power_w(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_max_power_w min_power_w(::my_custom_interfaces::msg::GpuMetrics::_min_power_w_type arg)
  {
    msg_.min_power_w = std::move(arg);
    return Init_GpuMetrics_max_power_w(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_avg_memory_mb
{
public:
  explicit Init_GpuMetrics_avg_memory_mb(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_min_power_w avg_memory_mb(::my_custom_interfaces::msg::GpuMetrics::_avg_memory_mb_type arg)
  {
    msg_.avg_memory_mb = std::move(arg);
    return Init_GpuMetrics_min_power_w(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_max_memory_mb
{
public:
  explicit Init_GpuMetrics_max_memory_mb(::my_custom_interfaces::msg::GpuMetrics & msg)
  : msg_(msg)
  {}
  Init_GpuMetrics_avg_memory_mb max_memory_mb(::my_custom_interfaces::msg::GpuMetrics::_max_memory_mb_type arg)
  {
    msg_.max_memory_mb = std::move(arg);
    return Init_GpuMetrics_avg_memory_mb(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

class Init_GpuMetrics_min_memory_mb
{
public:
  Init_GpuMetrics_min_memory_mb()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GpuMetrics_max_memory_mb min_memory_mb(::my_custom_interfaces::msg::GpuMetrics::_min_memory_mb_type arg)
  {
    msg_.min_memory_mb = std::move(arg);
    return Init_GpuMetrics_max_memory_mb(msg_);
  }

private:
  ::my_custom_interfaces::msg::GpuMetrics msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::my_custom_interfaces::msg::GpuMetrics>()
{
  return my_custom_interfaces::msg::builder::Init_GpuMetrics_min_memory_mb();
}

}  // namespace my_custom_interfaces

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__BUILDER_HPP_
