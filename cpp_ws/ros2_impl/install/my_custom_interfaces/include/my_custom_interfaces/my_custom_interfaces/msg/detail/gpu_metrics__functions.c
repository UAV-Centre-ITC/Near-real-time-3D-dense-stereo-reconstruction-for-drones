// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice
#include "my_custom_interfaces/msg/detail/gpu_metrics__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `stamp`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
my_custom_interfaces__msg__GpuMetrics__init(my_custom_interfaces__msg__GpuMetrics * msg)
{
  if (!msg) {
    return false;
  }
  // min_memory_mb
  // max_memory_mb
  // avg_memory_mb
  // min_power_w
  // max_power_w
  // avg_power_w
  // pcie_tx_bw_percentage
  // pcie_rx_bw_percentage
  // stamp
  if (!builtin_interfaces__msg__Time__init(&msg->stamp)) {
    my_custom_interfaces__msg__GpuMetrics__fini(msg);
    return false;
  }
  return true;
}

void
my_custom_interfaces__msg__GpuMetrics__fini(my_custom_interfaces__msg__GpuMetrics * msg)
{
  if (!msg) {
    return;
  }
  // min_memory_mb
  // max_memory_mb
  // avg_memory_mb
  // min_power_w
  // max_power_w
  // avg_power_w
  // pcie_tx_bw_percentage
  // pcie_rx_bw_percentage
  // stamp
  builtin_interfaces__msg__Time__fini(&msg->stamp);
}

bool
my_custom_interfaces__msg__GpuMetrics__are_equal(const my_custom_interfaces__msg__GpuMetrics * lhs, const my_custom_interfaces__msg__GpuMetrics * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // min_memory_mb
  if (lhs->min_memory_mb != rhs->min_memory_mb) {
    return false;
  }
  // max_memory_mb
  if (lhs->max_memory_mb != rhs->max_memory_mb) {
    return false;
  }
  // avg_memory_mb
  if (lhs->avg_memory_mb != rhs->avg_memory_mb) {
    return false;
  }
  // min_power_w
  if (lhs->min_power_w != rhs->min_power_w) {
    return false;
  }
  // max_power_w
  if (lhs->max_power_w != rhs->max_power_w) {
    return false;
  }
  // avg_power_w
  if (lhs->avg_power_w != rhs->avg_power_w) {
    return false;
  }
  // pcie_tx_bw_percentage
  if (lhs->pcie_tx_bw_percentage != rhs->pcie_tx_bw_percentage) {
    return false;
  }
  // pcie_rx_bw_percentage
  if (lhs->pcie_rx_bw_percentage != rhs->pcie_rx_bw_percentage) {
    return false;
  }
  // stamp
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->stamp), &(rhs->stamp)))
  {
    return false;
  }
  return true;
}

bool
my_custom_interfaces__msg__GpuMetrics__copy(
  const my_custom_interfaces__msg__GpuMetrics * input,
  my_custom_interfaces__msg__GpuMetrics * output)
{
  if (!input || !output) {
    return false;
  }
  // min_memory_mb
  output->min_memory_mb = input->min_memory_mb;
  // max_memory_mb
  output->max_memory_mb = input->max_memory_mb;
  // avg_memory_mb
  output->avg_memory_mb = input->avg_memory_mb;
  // min_power_w
  output->min_power_w = input->min_power_w;
  // max_power_w
  output->max_power_w = input->max_power_w;
  // avg_power_w
  output->avg_power_w = input->avg_power_w;
  // pcie_tx_bw_percentage
  output->pcie_tx_bw_percentage = input->pcie_tx_bw_percentage;
  // pcie_rx_bw_percentage
  output->pcie_rx_bw_percentage = input->pcie_rx_bw_percentage;
  // stamp
  if (!builtin_interfaces__msg__Time__copy(
      &(input->stamp), &(output->stamp)))
  {
    return false;
  }
  return true;
}

my_custom_interfaces__msg__GpuMetrics *
my_custom_interfaces__msg__GpuMetrics__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  my_custom_interfaces__msg__GpuMetrics * msg = (my_custom_interfaces__msg__GpuMetrics *)allocator.allocate(sizeof(my_custom_interfaces__msg__GpuMetrics), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(my_custom_interfaces__msg__GpuMetrics));
  bool success = my_custom_interfaces__msg__GpuMetrics__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
my_custom_interfaces__msg__GpuMetrics__destroy(my_custom_interfaces__msg__GpuMetrics * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    my_custom_interfaces__msg__GpuMetrics__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
my_custom_interfaces__msg__GpuMetrics__Sequence__init(my_custom_interfaces__msg__GpuMetrics__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  my_custom_interfaces__msg__GpuMetrics * data = NULL;

  if (size) {
    data = (my_custom_interfaces__msg__GpuMetrics *)allocator.zero_allocate(size, sizeof(my_custom_interfaces__msg__GpuMetrics), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = my_custom_interfaces__msg__GpuMetrics__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        my_custom_interfaces__msg__GpuMetrics__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
my_custom_interfaces__msg__GpuMetrics__Sequence__fini(my_custom_interfaces__msg__GpuMetrics__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      my_custom_interfaces__msg__GpuMetrics__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

my_custom_interfaces__msg__GpuMetrics__Sequence *
my_custom_interfaces__msg__GpuMetrics__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  my_custom_interfaces__msg__GpuMetrics__Sequence * array = (my_custom_interfaces__msg__GpuMetrics__Sequence *)allocator.allocate(sizeof(my_custom_interfaces__msg__GpuMetrics__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = my_custom_interfaces__msg__GpuMetrics__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
my_custom_interfaces__msg__GpuMetrics__Sequence__destroy(my_custom_interfaces__msg__GpuMetrics__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    my_custom_interfaces__msg__GpuMetrics__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
my_custom_interfaces__msg__GpuMetrics__Sequence__are_equal(const my_custom_interfaces__msg__GpuMetrics__Sequence * lhs, const my_custom_interfaces__msg__GpuMetrics__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!my_custom_interfaces__msg__GpuMetrics__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
my_custom_interfaces__msg__GpuMetrics__Sequence__copy(
  const my_custom_interfaces__msg__GpuMetrics__Sequence * input,
  my_custom_interfaces__msg__GpuMetrics__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(my_custom_interfaces__msg__GpuMetrics);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    my_custom_interfaces__msg__GpuMetrics * data =
      (my_custom_interfaces__msg__GpuMetrics *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!my_custom_interfaces__msg__GpuMetrics__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          my_custom_interfaces__msg__GpuMetrics__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!my_custom_interfaces__msg__GpuMetrics__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
