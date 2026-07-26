// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from my_custom_interfaces:msg/QmatrixStamped.idl
// generated code does not contain a copyright notice
#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_


#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "my_custom_interfaces/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "my_custom_interfaces/msg/detail/qmatrix_stamped__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
bool cdr_serialize_my_custom_interfaces__msg__QmatrixStamped(
  const my_custom_interfaces__msg__QmatrixStamped * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
bool cdr_deserialize_my_custom_interfaces__msg__QmatrixStamped(
  eprosima::fastcdr::Cdr &,
  my_custom_interfaces__msg__QmatrixStamped * ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
size_t get_serialized_size_my_custom_interfaces__msg__QmatrixStamped(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
size_t max_serialized_size_my_custom_interfaces__msg__QmatrixStamped(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
bool cdr_serialize_key_my_custom_interfaces__msg__QmatrixStamped(
  const my_custom_interfaces__msg__QmatrixStamped * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
size_t get_serialized_size_key_my_custom_interfaces__msg__QmatrixStamped(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
size_t max_serialized_size_key_my_custom_interfaces__msg__QmatrixStamped(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_my_custom_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, my_custom_interfaces, msg, QmatrixStamped)();

#ifdef __cplusplus
}
#endif

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__QMATRIX_STAMPED__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
