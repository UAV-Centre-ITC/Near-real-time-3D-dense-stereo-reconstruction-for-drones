// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "my_custom_interfaces/msg/detail/gpu_metrics__rosidl_typesupport_introspection_c.h"
#include "my_custom_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "my_custom_interfaces/msg/detail/gpu_metrics__functions.h"
#include "my_custom_interfaces/msg/detail/gpu_metrics__struct.h"


// Include directives for member types
// Member `stamp`
#include "builtin_interfaces/msg/time.h"
// Member `stamp`
#include "builtin_interfaces/msg/detail/time__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  my_custom_interfaces__msg__GpuMetrics__init(message_memory);
}

void my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_fini_function(void * message_memory)
{
  my_custom_interfaces__msg__GpuMetrics__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_member_array[9] = {
  {
    "min_memory_mb",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT64,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, min_memory_mb),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "max_memory_mb",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT64,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, max_memory_mb),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "avg_memory_mb",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, avg_memory_mb),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "min_power_w",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, min_power_w),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "max_power_w",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, max_power_w),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "avg_power_w",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, avg_power_w),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "pcie_tx_bw_percentage",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, pcie_tx_bw_percentage),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "pcie_rx_bw_percentage",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, pcie_rx_bw_percentage),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "stamp",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(my_custom_interfaces__msg__GpuMetrics, stamp),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_members = {
  "my_custom_interfaces__msg",  // message namespace
  "GpuMetrics",  // message name
  9,  // number of fields
  sizeof(my_custom_interfaces__msg__GpuMetrics),
  false,  // has_any_key_member_
  my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_member_array,  // message members
  my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_init_function,  // function to initialize message memory (memory has to be allocated)
  my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_type_support_handle = {
  0,
  &my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_members,
  get_message_typesupport_handle_function,
  &my_custom_interfaces__msg__GpuMetrics__get_type_hash,
  &my_custom_interfaces__msg__GpuMetrics__get_type_description,
  &my_custom_interfaces__msg__GpuMetrics__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_my_custom_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, my_custom_interfaces, msg, GpuMetrics)() {
  my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_member_array[8].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, builtin_interfaces, msg, Time)();
  if (!my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_type_support_handle.typesupport_identifier) {
    my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &my_custom_interfaces__msg__GpuMetrics__rosidl_typesupport_introspection_c__GpuMetrics_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
