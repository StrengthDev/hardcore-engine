//! Structures and associated traits which hold data to be used by the renderer.

pub use texture::*;
pub use vertex::*;

pub mod buffer;
pub mod descriptor;
mod texture;

mod descriptor_macros;
mod vertex;
