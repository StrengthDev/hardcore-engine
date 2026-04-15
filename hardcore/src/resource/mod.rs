//! Structures and associated traits which hold data to be used by the renderer.

pub use texture::*;
pub use vertex::*;

pub mod buffer;
pub mod descriptor;
mod texture;

mod descriptor_macros;
mod vertex;

/// How writes to this resource are synchronized for dependent operations.
#[derive(Default)]
pub enum Synchronization {
    /// When a resource is written to, operations will block and wait until it is updated and ready
    /// to be used before they execute.
    #[default]
    Immediate,

    /// When a resource is written to, operations will not wait and instead use the resource with
    /// its previous values. Only when the resource has been completely updated and is ready for
    /// use, will the new values be used by dependent operations.
    ///
    /// This strategy requires additional memory to be allocated for this resource.
    Async,
}
