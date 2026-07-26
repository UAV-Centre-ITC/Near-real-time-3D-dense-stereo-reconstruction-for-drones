// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "my_custom_interfaces/msg/gpu_metrics.h"


#ifndef MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_H_
#define MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'stamp'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/GpuMetrics in the package my_custom_interfaces.
typedef struct my_custom_interfaces__msg__GpuMetrics
{
  uint64_t min_memory_mb;
  uint64_t max_memory_mb;
  float avg_memory_mb;
  float min_power_w;
  float max_power_w;
  float avg_power_w;
  float pcie_tx_bw_percentage;
  float pcie_rx_bw_percentage;
  /// time at which the data is measured
  builtin_interfaces__msg__Time stamp;
} my_custom_interfaces__msg__GpuMetrics;

// Struct for a sequence of my_custom_interfaces__msg__GpuMetrics.
typedef struct my_custom_interfaces__msg__GpuMetrics__Sequence
{
  my_custom_interfaces__msg__GpuMetrics * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} my_custom_interfaces__msg__GpuMetrics__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MY_CUSTOM_INTERFACES__MSG__DETAIL__GPU_METRICS__STRUCT_H_
