// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/gpu_metrics.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__TRAITS_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "my_custom_interfaces/msg/detail/gpu_metrics__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace my_custom_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const GpuMetrics & msg,
  std::ostream & out)
{
  out << "{";
  // member: min_memory_mb
  {
    out << "min_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.min_memory_mb, out);
    out << ", ";
  }

  // member: max_memory_mb
  {
    out << "max_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.max_memory_mb, out);
    out << ", ";
  }

  // member: avg_memory_mb
  {
    out << "avg_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.avg_memory_mb, out);
    out << ", ";
  }

  // member: min_power_w
  {
    out << "min_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.min_power_w, out);
    out << ", ";
  }

  // member: max_power_w
  {
    out << "max_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.max_power_w, out);
    out << ", ";
  }

  // member: avg_power_w
  {
    out << "avg_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.avg_power_w, out);
    out << ", ";
  }

  // member: pcie_tx_bw_percentage
  {
    out << "pcie_tx_bw_percentage: ";
    rosidl_generator_traits::value_to_yaml(msg.pcie_tx_bw_percentage, out);
    out << ", ";
  }

  // member: pcie_rx_bw_percentage
  {
    out << "pcie_rx_bw_percentage: ";
    rosidl_generator_traits::value_to_yaml(msg.pcie_rx_bw_percentage, out);
    out << ", ";
  }

  // member: stamp
  {
    out << "stamp: ";
    to_flow_style_yaml(msg.stamp, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const GpuMetrics & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: min_memory_mb
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "min_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.min_memory_mb, out);
    out << "\n";
  }

  // member: max_memory_mb
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "max_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.max_memory_mb, out);
    out << "\n";
  }

  // member: avg_memory_mb
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "avg_memory_mb: ";
    rosidl_generator_traits::value_to_yaml(msg.avg_memory_mb, out);
    out << "\n";
  }

  // member: min_power_w
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "min_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.min_power_w, out);
    out << "\n";
  }

  // member: max_power_w
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "max_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.max_power_w, out);
    out << "\n";
  }

  // member: avg_power_w
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "avg_power_w: ";
    rosidl_generator_traits::value_to_yaml(msg.avg_power_w, out);
    out << "\n";
  }

  // member: pcie_tx_bw_percentage
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "pcie_tx_bw_percentage: ";
    rosidl_generator_traits::value_to_yaml(msg.pcie_tx_bw_percentage, out);
    out << "\n";
  }

  // member: pcie_rx_bw_percentage
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "pcie_rx_bw_percentage: ";
    rosidl_generator_traits::value_to_yaml(msg.pcie_rx_bw_percentage, out);
    out << "\n";
  }

  // member: stamp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "stamp:\n";
    to_block_style_yaml(msg.stamp, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const GpuMetrics & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace my_custom_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use my_custom_interfaces::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const my_custom_interfaces::msg::GpuMetrics & msg,
  std::ostream & out, size_t indentation = 0)
{
  my_custom_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use my_custom_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const my_custom_interfaces::msg::GpuMetrics & msg)
{
  return my_custom_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<my_custom_interfaces::msg::GpuMetrics>()
{
  return "my_custom_interfaces::msg::GpuMetrics";
}

template<>
inline const char * name<my_custom_interfaces::msg::GpuMetrics>()
{
  return "my_custom_interfaces/msg/GpuMetrics";
}

template<>
struct has_fixed_size<my_custom_interfaces::msg::GpuMetrics>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct has_bounded_size<my_custom_interfaces::msg::GpuMetrics>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct is_message<my_custom_interfaces::msg::GpuMetrics>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__TRAITS_HPP_
