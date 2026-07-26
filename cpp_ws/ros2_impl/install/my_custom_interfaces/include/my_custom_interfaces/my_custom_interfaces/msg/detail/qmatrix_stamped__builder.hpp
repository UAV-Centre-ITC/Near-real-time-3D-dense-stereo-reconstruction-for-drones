// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from my_custom_interfaces:msg/QmatrixStamped.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/qmatrix_stamped.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__BUILDER_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "my_custom_interfaces/msg/detail/qmatrix_stamped__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace my_custom_interfaces
{

namespace msg
{

namespace builder
{

class Init_QmatrixStamped_header
{
public:
  explicit Init_QmatrixStamped_header(::my_custom_interfaces::msg::QmatrixStamped & msg)
  : msg_(msg)
  {}
  ::my_custom_interfaces::msg::QmatrixStamped header(::my_custom_interfaces::msg::QmatrixStamped::_header_type arg)
  {
    msg_.header = std::move(arg);
    return std::move(msg_);
  }

private:
  ::my_custom_interfaces::msg::QmatrixStamped msg_;
};

class Init_QmatrixStamped_data
{
public:
  Init_QmatrixStamped_data()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_QmatrixStamped_header data(::my_custom_interfaces::msg::QmatrixStamped::_data_type arg)
  {
    msg_.data = std::move(arg);
    return Init_QmatrixStamped_header(msg_);
  }

private:
  ::my_custom_interfaces::msg::QmatrixStamped msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::my_custom_interfaces::msg::QmatrixStamped>()
{
  return my_custom_interfaces::msg::builder::Init_QmatrixStamped_data();
}

}  // namespace my_custom_interfaces

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__BUILDER_HPP_
