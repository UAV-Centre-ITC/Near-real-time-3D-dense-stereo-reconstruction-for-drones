// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from my_custom_interfaces:msg/QmatrixStamped.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/qmatrix_stamped.hpp"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__TRAITS_HPP_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "my_custom_interfaces/msg/detail/qmatrix_stamped__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"

namespace my_custom_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const QmatrixStamped & msg,
  std::ostream & out)
{
  out << "{";
  // member: data
  {
    if (msg.data.size() == 0) {
      out << "data: []";
    } else {
      out << "data: [";
      size_t pending_items = msg.data.size();
      for (auto item : msg.data) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const QmatrixStamped & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: data
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.data.size() == 0) {
      out << "data: []\n";
    } else {
      out << "data:\n";
      for (auto item : msg.data) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: header
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "header:\n";
    to_block_style_yaml(msg.header, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const QmatrixStamped & msg, bool use_flow_style = false)
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
  const my_custom_interfaces::msg::QmatrixStamped & msg,
  std::ostream & out, size_t indentation = 0)
{
  my_custom_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use my_custom_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const my_custom_interfaces::msg::QmatrixStamped & msg)
{
  return my_custom_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<my_custom_interfaces::msg::QmatrixStamped>()
{
  return "my_custom_interfaces::msg::QmatrixStamped";
}

template<>
inline const char * name<my_custom_interfaces::msg::QmatrixStamped>()
{
  return "my_custom_interfaces/msg/QmatrixStamped";
}

template<>
struct has_fixed_size<my_custom_interfaces::msg::QmatrixStamped>
  : std::integral_constant<bool, has_fixed_size<std_msgs::msg::Header>::value> {};

template<>
struct has_bounded_size<my_custom_interfaces::msg::QmatrixStamped>
  : std::integral_constant<bool, has_bounded_size<std_msgs::msg::Header>::value> {};

template<>
struct is_message<my_custom_interfaces::msg::QmatrixStamped>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__TRAITS_HPP_
