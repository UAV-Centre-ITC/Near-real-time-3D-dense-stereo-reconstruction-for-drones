// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/gpu_metrics.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__my_custom_interfaces__msg__GpuMetrics __attribute__((deprecated))
#else
# define DEPRECATED__my_custom_interfaces__msg__GpuMetrics __declspec(deprecated)
#endif

namespace my_custom_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GpuMetrics_
{
  using Type = GpuMetrics_<ContainerAllocator>;

  explicit GpuMetrics_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->min_memory_mb = 0ull;
      this->max_memory_mb = 0ull;
      this->avg_memory_mb = 0.0f;
      this->min_power_w = 0.0f;
      this->max_power_w = 0.0f;
      this->avg_power_w = 0.0f;
      this->pcie_tx_bw_percentage = 0.0f;
      this->pcie_rx_bw_percentage = 0.0f;
    }
  }

  explicit GpuMetrics_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : stamp(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->min_memory_mb = 0ull;
      this->max_memory_mb = 0ull;
      this->avg_memory_mb = 0.0f;
      this->min_power_w = 0.0f;
      this->max_power_w = 0.0f;
      this->avg_power_w = 0.0f;
      this->pcie_tx_bw_percentage = 0.0f;
      this->pcie_rx_bw_percentage = 0.0f;
    }
  }

  // field types and members
  using _min_memory_mb_type =
    uint64_t;
  _min_memory_mb_type min_memory_mb;
  using _max_memory_mb_type =
    uint64_t;
  _max_memory_mb_type max_memory_mb;
  using _avg_memory_mb_type =
    float;
  _avg_memory_mb_type avg_memory_mb;
  using _min_power_w_type =
    float;
  _min_power_w_type min_power_w;
  using _max_power_w_type =
    float;
  _max_power_w_type max_power_w;
  using _avg_power_w_type =
    float;
  _avg_power_w_type avg_power_w;
  using _pcie_tx_bw_percentage_type =
    float;
  _pcie_tx_bw_percentage_type pcie_tx_bw_percentage;
  using _pcie_rx_bw_percentage_type =
    float;
  _pcie_rx_bw_percentage_type pcie_rx_bw_percentage;
  using _stamp_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _stamp_type stamp;

  // setters for named parameter idiom
  Type & set__min_memory_mb(
    const uint64_t & _arg)
  {
    this->min_memory_mb = _arg;
    return *this;
  }
  Type & set__max_memory_mb(
    const uint64_t & _arg)
  {
    this->max_memory_mb = _arg;
    return *this;
  }
  Type & set__avg_memory_mb(
    const float & _arg)
  {
    this->avg_memory_mb = _arg;
    return *this;
  }
  Type & set__min_power_w(
    const float & _arg)
  {
    this->min_power_w = _arg;
    return *this;
  }
  Type & set__max_power_w(
    const float & _arg)
  {
    this->max_power_w = _arg;
    return *this;
  }
  Type & set__avg_power_w(
    const float & _arg)
  {
    this->avg_power_w = _arg;
    return *this;
  }
  Type & set__pcie_tx_bw_percentage(
    const float & _arg)
  {
    this->pcie_tx_bw_percentage = _arg;
    return *this;
  }
  Type & set__pcie_rx_bw_percentage(
    const float & _arg)
  {
    this->pcie_rx_bw_percentage = _arg;
    return *this;
  }
  Type & set__stamp(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->stamp = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> *;
  using ConstRawPtr =
    const my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__my_custom_interfaces__msg__GpuMetrics
    std::shared_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__my_custom_interfaces__msg__GpuMetrics
    std::shared_ptr<my_custom_interfaces::msg::GpuMetrics_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GpuMetrics_ & other) const
  {
    if (this->min_memory_mb != other.min_memory_mb) {
      return false;
    }
    if (this->max_memory_mb != other.max_memory_mb) {
      return false;
    }
    if (this->avg_memory_mb != other.avg_memory_mb) {
      return false;
    }
    if (this->min_power_w != other.min_power_w) {
      return false;
    }
    if (this->max_power_w != other.max_power_w) {
      return false;
    }
    if (this->avg_power_w != other.avg_power_w) {
      return false;
    }
    if (this->pcie_tx_bw_percentage != other.pcie_tx_bw_percentage) {
      return false;
    }
    if (this->pcie_rx_bw_percentage != other.pcie_rx_bw_percentage) {
      return false;
    }
    if (this->stamp != other.stamp) {
      return false;
    }
    return true;
  }
  bool operator!=(const GpuMetrics_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GpuMetrics_

// alias to use template instance with default allocator
using GpuMetrics =
  my_custom_interfaces::msg::GpuMetrics_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace my_custom_interfaces

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_HPP_
