// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from my_custom_interfaces:msg/QmatrixStamped.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/qmatrix_stamped.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__my_custom_interfaces__msg__QmatrixStamped __attribute__((deprecated))
#else
# define DEPRECATED__my_custom_interfaces__msg__QmatrixStamped __declspec(deprecated)
#endif

namespace my_custom_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct QmatrixStamped_
{
  using Type = QmatrixStamped_<ContainerAllocator>;

  explicit QmatrixStamped_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<double, 16>::iterator, double>(this->data.begin(), this->data.end(), 0.0);
    }
  }

  explicit QmatrixStamped_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : data(_alloc),
    header(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<double, 16>::iterator, double>(this->data.begin(), this->data.end(), 0.0);
    }
  }

  // field types and members
  using _data_type =
    std::array<double, 16>;
  _data_type data;
  using _header_type =
    std_msgs::msg::Header_<ContainerAllocator>;
  _header_type header;

  // setters for named parameter idiom
  Type & set__data(
    const std::array<double, 16> & _arg)
  {
    this->data = _arg;
    return *this;
  }
  Type & set__header(
    const std_msgs::msg::Header_<ContainerAllocator> & _arg)
  {
    this->header = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> *;
  using ConstRawPtr =
    const my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__my_custom_interfaces__msg__QmatrixStamped
    std::shared_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__my_custom_interfaces__msg__QmatrixStamped
    std::shared_ptr<my_custom_interfaces::msg::QmatrixStamped_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const QmatrixStamped_ & other) const
  {
    if (this->data != other.data) {
      return false;
    }
    if (this->header != other.header) {
      return false;
    }
    return true;
  }
  bool operator!=(const QmatrixStamped_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct QmatrixStamped_

// alias to use template instance with default allocator
using QmatrixStamped =
  my_custom_interfaces::msg::QmatrixStamped_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace my_custom_interfaces

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_HPP_
