#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "my_custom_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__my_custom_interfaces__msg__QmatrixStamped() -> *const std::ffi::c_void;
}

#[link(name = "my_custom_interfaces__rosidl_generator_c")]
extern "C" {
    fn my_custom_interfaces__msg__QmatrixStamped__init(msg: *mut QmatrixStamped) -> bool;
    fn my_custom_interfaces__msg__QmatrixStamped__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<QmatrixStamped>, size: usize) -> bool;
    fn my_custom_interfaces__msg__QmatrixStamped__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<QmatrixStamped>);
    fn my_custom_interfaces__msg__QmatrixStamped__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<QmatrixStamped>, out_seq: *mut rosidl_runtime_rs::Sequence<QmatrixStamped>) -> bool;
}

// Corresponds to my_custom_interfaces__msg__QmatrixStamped
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct QmatrixStamped {
    /// Row-major representation of the 4x4  Q matrix
    pub data: [f64; 16],

    /// Header for timestamp synchronization
    pub header: std_msgs::msg::rmw::Header,

}



impl Default for QmatrixStamped {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !my_custom_interfaces__msg__QmatrixStamped__init(&mut msg as *mut _) {
        panic!("Call to my_custom_interfaces__msg__QmatrixStamped__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for QmatrixStamped {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__QmatrixStamped__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__QmatrixStamped__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__QmatrixStamped__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for QmatrixStamped {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for QmatrixStamped where Self: Sized {
  const TYPE_NAME: &'static str = "my_custom_interfaces/msg/QmatrixStamped";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__my_custom_interfaces__msg__QmatrixStamped() }
  }
}


#[link(name = "my_custom_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__my_custom_interfaces__msg__GpuMetrics() -> *const std::ffi::c_void;
}

#[link(name = "my_custom_interfaces__rosidl_generator_c")]
extern "C" {
    fn my_custom_interfaces__msg__GpuMetrics__init(msg: *mut GpuMetrics) -> bool;
    fn my_custom_interfaces__msg__GpuMetrics__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GpuMetrics>, size: usize) -> bool;
    fn my_custom_interfaces__msg__GpuMetrics__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GpuMetrics>);
    fn my_custom_interfaces__msg__GpuMetrics__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GpuMetrics>, out_seq: *mut rosidl_runtime_rs::Sequence<GpuMetrics>) -> bool;
}

// Corresponds to my_custom_interfaces__msg__GpuMetrics
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GpuMetrics {

    // This member is not documented.
    #[allow(missing_docs)]
    pub min_memory_mb: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub max_memory_mb: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub avg_memory_mb: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub min_power_w: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub max_power_w: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub avg_power_w: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub pcie_tx_bw_percentage: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub pcie_rx_bw_percentage: f32,

    /// time at which the data is measured
    pub stamp: builtin_interfaces::msg::rmw::Time,

}



impl Default for GpuMetrics {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !my_custom_interfaces__msg__GpuMetrics__init(&mut msg as *mut _) {
        panic!("Call to my_custom_interfaces__msg__GpuMetrics__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GpuMetrics {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__GpuMetrics__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__GpuMetrics__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { my_custom_interfaces__msg__GpuMetrics__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GpuMetrics {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GpuMetrics where Self: Sized {
  const TYPE_NAME: &'static str = "my_custom_interfaces/msg/GpuMetrics";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__my_custom_interfaces__msg__GpuMetrics() }
  }
}


