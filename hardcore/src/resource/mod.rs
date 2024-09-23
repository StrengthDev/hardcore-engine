//! Structures and associated traits which hold data to be used by the renderer.

use std::ops::{Deref, DerefMut};

pub use mesh::*;
pub use texture::*;

mod buffer;
pub mod descriptor;
mod generic;
mod mesh;
mod texture;
pub mod vertex;

/// Structures holding device local data only directly accessible by the device.
pub mod local {
    pub use crate::resource::generic::local::*;
}

/// Structures holding device local data which may be directly accessed by the host CPU.
pub mod dynamic {
    pub use crate::resource::generic::local::*;
}

/// Structures holding host data which may be accessed by the device.
pub mod host {
    pub use crate::resource::generic::local::*;
}

pub trait DeviceResource {}

pub trait HostData: Deref + DerefMut {}
