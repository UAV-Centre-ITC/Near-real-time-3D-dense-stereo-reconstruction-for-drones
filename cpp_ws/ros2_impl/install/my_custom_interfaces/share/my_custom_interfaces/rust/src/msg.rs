#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to my_custom_interfaces__msg__QmatrixStamped

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct QmatrixStamped {
    /// Row-major representation of the 4x4  Q matrix
    pub data: [f64; 16],

    /// Header for timestamp synchronization
    pub header: std_msgs::msg::Header,

}



impl Default for QmatrixStamped {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::QmatrixStamped::default())
  }
}

impl rosidl_runtime_rs::Message for QmatrixStamped {
  type RmwMsg = super::msg::rmw::QmatrixStamped;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        data: msg.data,
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        data: msg.data,
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      data: msg.data,
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
    }
  }
}


// Corresponds to my_custom_interfaces__msg__GpuMetrics

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
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
    pub stamp: builtin_interfaces::msg::Time,

}



impl Default for GpuMetrics {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GpuMetrics::default())
  }
}

impl rosidl_runtime_rs::Message for GpuMetrics {
  type RmwMsg = super::msg::rmw::GpuMetrics;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        min_memory_mb: msg.min_memory_mb,
        max_memory_mb: msg.max_memory_mb,
        avg_memory_mb: msg.avg_memory_mb,
        min_power_w: msg.min_power_w,
        max_power_w: msg.max_power_w,
        avg_power_w: msg.avg_power_w,
        pcie_tx_bw_percentage: msg.pcie_tx_bw_percentage,
        pcie_rx_bw_percentage: msg.pcie_rx_bw_percentage,
        stamp: builtin_interfaces::msg::Time::into_rmw_message(std::borrow::Cow::Owned(msg.stamp)).into_owned(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      min_memory_mb: msg.min_memory_mb,
      max_memory_mb: msg.max_memory_mb,
      avg_memory_mb: msg.avg_memory_mb,
      min_power_w: msg.min_power_w,
      max_power_w: msg.max_power_w,
      avg_power_w: msg.avg_power_w,
      pcie_tx_bw_percentage: msg.pcie_tx_bw_percentage,
      pcie_rx_bw_percentage: msg.pcie_rx_bw_percentage,
        stamp: builtin_interfaces::msg::Time::into_rmw_message(std::borrow::Cow::Borrowed(&msg.stamp)).into_owned(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      min_memory_mb: msg.min_memory_mb,
      max_memory_mb: msg.max_memory_mb,
      avg_memory_mb: msg.avg_memory_mb,
      min_power_w: msg.min_power_w,
      max_power_w: msg.max_power_w,
      avg_power_w: msg.avg_power_w,
      pcie_tx_bw_percentage: msg.pcie_tx_bw_percentage,
      pcie_rx_bw_percentage: msg.pcie_rx_bw_percentage,
      stamp: builtin_interfaces::msg::Time::from_rmw_message(msg.stamp),
    }
  }
}


