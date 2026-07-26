// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from my_custom_interfaces:msg/GpuMetrics.idl
// generated code does not contain a copyright notice

#include "my_custom_interfaces/msg/detail/gpu_metrics__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_my_custom_interfaces
const rosidl_type_hash_t *
my_custom_interfaces__msg__GpuMetrics__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x04, 0xa7, 0xf3, 0xf7, 0x8d, 0x8d, 0x60, 0xb5,
      0x26, 0x2f, 0x74, 0x80, 0xd5, 0x74, 0xb6, 0x6c,
      0x11, 0xc2, 0x45, 0x17, 0x51, 0xd3, 0xf8, 0xe1,
      0xc7, 0x33, 0x52, 0x9b, 0xb7, 0xb6, 0x20, 0x6d,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types
#include "builtin_interfaces/msg/detail/time__functions.h"

// Hashes for external referenced types
#ifndef NDEBUG
static const rosidl_type_hash_t builtin_interfaces__msg__Time__EXPECTED_HASH = {1, {
    0xb1, 0x06, 0x23, 0x5e, 0x25, 0xa4, 0xc5, 0xed,
    0x35, 0x09, 0x8a, 0xa0, 0xa6, 0x1a, 0x3e, 0xe9,
    0xc9, 0xb1, 0x8d, 0x19, 0x7f, 0x39, 0x8b, 0x0e,
    0x42, 0x06, 0xce, 0xa9, 0xac, 0xf9, 0xc1, 0x97,
  }};
#endif

static char my_custom_interfaces__msg__GpuMetrics__TYPE_NAME[] = "my_custom_interfaces/msg/GpuMetrics";
static char builtin_interfaces__msg__Time__TYPE_NAME[] = "builtin_interfaces/msg/Time";

// Define type names, field names, and default values
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__min_memory_mb[] = "min_memory_mb";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__max_memory_mb[] = "max_memory_mb";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__avg_memory_mb[] = "avg_memory_mb";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__min_power_w[] = "min_power_w";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__max_power_w[] = "max_power_w";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__avg_power_w[] = "avg_power_w";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__pcie_tx_bw_percentage[] = "pcie_tx_bw_percentage";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__pcie_rx_bw_percentage[] = "pcie_rx_bw_percentage";
static char my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__stamp[] = "stamp";

static rosidl_runtime_c__type_description__Field my_custom_interfaces__msg__GpuMetrics__FIELDS[] = {
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__min_memory_mb, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT64,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__max_memory_mb, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT64,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__avg_memory_mb, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__min_power_w, 11, 11},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__max_power_w, 11, 11},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__avg_power_w, 11, 11},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__pcie_tx_bw_percentage, 21, 21},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__pcie_rx_bw_percentage, 21, 21},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {my_custom_interfaces__msg__GpuMetrics__FIELD_NAME__stamp, 5, 5},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    },
    {NULL, 0, 0},
  },
};

static rosidl_runtime_c__type_description__IndividualTypeDescription my_custom_interfaces__msg__GpuMetrics__REFERENCED_TYPE_DESCRIPTIONS[] = {
  {
    {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
my_custom_interfaces__msg__GpuMetrics__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {my_custom_interfaces__msg__GpuMetrics__TYPE_NAME, 35, 35},
      {my_custom_interfaces__msg__GpuMetrics__FIELDS, 9, 9},
    },
    {my_custom_interfaces__msg__GpuMetrics__REFERENCED_TYPE_DESCRIPTIONS, 1, 1},
  };
  if (!constructed) {
    assert(0 == memcmp(&builtin_interfaces__msg__Time__EXPECTED_HASH, builtin_interfaces__msg__Time__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[0].fields = builtin_interfaces__msg__Time__get_type_description(NULL)->type_description.fields;
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "uint64 min_memory_mb\n"
  "uint64 max_memory_mb\n"
  "float32 avg_memory_mb\n"
  "float32 min_power_w\n"
  "float32 max_power_w\n"
  "float32 avg_power_w\n"
  "float32 pcie_tx_bw_percentage\n"
  "float32 pcie_rx_bw_percentage\n"
  "builtin_interfaces/Time stamp # time at which the data is measured\n"
  "\n"
  "";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
my_custom_interfaces__msg__GpuMetrics__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {my_custom_interfaces__msg__GpuMetrics__TYPE_NAME, 35, 35},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 253, 253},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
my_custom_interfaces__msg__GpuMetrics__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[2];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 2, 2};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *my_custom_interfaces__msg__GpuMetrics__get_individual_type_description_source(NULL),
    sources[1] = *builtin_interfaces__msg__Time__get_individual_type_description_source(NULL);
    constructed = true;
  }
  return &source_sequence;
}
