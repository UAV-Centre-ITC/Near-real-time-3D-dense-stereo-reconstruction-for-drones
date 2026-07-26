// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from my_custom_interfaces:msg/QmatrixStamped.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/qmatrix_stamped.h"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_H_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.h"

/// Struct defined in msg/QmatrixStamped in the package my_custom_interfaces.
typedef struct my_custom_interfaces__msg__QmatrixStamped
{
  /// Row-major representation of the 4x4  Q matrix
  double data[16];
  /// Header for timestamp synchronization
  std_msgs__msg__Header header;
} my_custom_interfaces__msg__QmatrixStamped;

// Struct for a sequence of my_custom_interfaces__msg__QmatrixStamped.
typedef struct my_custom_interfaces__msg__QmatrixStamped__Sequence
{
  my_custom_interfaces__msg__QmatrixStamped * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} my_custom_interfaces__msg__QmatrixStamped__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__STRUCT_H_
