//! Automatically generated **Rust** bindings to **Hardcore**'s native module.

#![warn(missing_docs)]

extern crate link_cplusplus;

use std::fmt::Formatter;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::GLFWInitFailed => write!(f, "Failed to initialize global GLFW context."),
            Error::VulkanInitFailed => write!(f, "Failed to initialize global Vulkan context."),
            Error::VulkanLayerNotFound => write!(f, "One or more required Vulkan layers were not found."),
            Error::VulkanExtensionNotFound => write!(f, "One or more required Vulkan extensions were not found."),
            Error::OutOfHostMemory => write!(f, "Out of host memory."),
            Error::OutOfDeviceMemory => write!(f, "Out of device memory."),
            Error::IncompatibleDriver => write!(f, "System driver is not compatible with the Vulkan version being used."),
            Error::NoDevices => write!(f, "No devices were found."),
            Error::NoSuchDevice => write!(f, "The specified device does not exist."),
            Error::InvalidParams => write!(f, "The provided parameters are not valid."),
            Error::GLFWWindowCreationFailure => write!(f, "Failed to create new GLFW window."),
            Error::NativeWindowInUse => write!(f, "The native window is already being used by another API."),
            Error::NoPresentSupport => write!(f, "Presentation of the surface is not supported by the specified device."),
            Error::DeviceLost => write!(f, "The specified device is no longer valid."),
            Error::SurfaceLost => write!(f, "The specified surface is no longer valid."),
            Error::CompressionExhausted => write!(f, "Internal resources required for compression are exhausted."),
            Error::InvalidOpaqueCaptureAddress => write!(f, "Invalid opaque capture address."),
            Error::InvalidExternalHandle => write!(f, "An external handle is not a valid handle of the specified type."),
            Error::FormatNotSupported => write!(f, "The provided format is not supported by the hardware."),
            Error::TextureParamsNotSupported => write!(f, "The provided texture parameters are not supported by the hardware for the given format."),
            Error::CouldNotFitInPool => write!(f, "Could not allocate the desired range in the specified memory pool."),
            Error::UnmetHeapRequirements => write!(f, "Hardware does not support required heap capabilities."),
            Error::MemoryMapFailed => write!(f, "Failed to map device memory."),
            Error::UnmetQueueRequirements => write!(f, "Hardware does not support required queue capabilities."),
            Error::InvalidRenderGraph => write!(f, "Attempted to use the render graph while it is in an invalid state."),
            Error::ShaderReflectionFailed => write!(f, "Failed to perform shader resource reflection."),
            Error::DescriptorMismatch => write!(f, "Two or more descriptors did not match."),
            Error::VulkanUnknown => write!(f, "Unknown Vulkan error, could be invalid input or an implementation failure."),
            Error::ValidationFailed => write!(f, "Command failed due to invalid usage."),
            Error::Fragmentation => write!(f, "Could not create descriptor pool due to fragmentation."),
            Error::InvalidShader => write!(f, "Invalid shader."),
            Error::NoSuchResource => write!(f, "The provided resource input does not exist."),
            Error::NoSuchNode => write!(f, "The provided execution graph node does not exist."),
            Error::PrunedNode => write!(f, "The provided execution graph node has deleted dependencies."),
        }
    }
}

impl core::error::Error for Error {}

impl From<Result> for core::result::Result<(), Error> {
    fn from(value: Result) -> Self {
        if value.success {
            Ok(())
        } else {
            Err(value.error)
        }
    }
}

impl Result {
    /// Convert into the standard result type.
    pub fn into_std_result(self) -> core::result::Result<(), Error> {
        self.into()
    }
}

// Having these aliases hardcoded here is really sad, but there's no better way to add the doc comments.
impl MouseButton {
    /// The left mouse button, an alias for mouse button 1.
    pub const LEFT: MouseButton = MouseButton::Button1;

    /// The right mouse button, an alias for mouse button 2.
    pub const RIGHT: MouseButton = MouseButton::Button2;

    /// The middle mouse button, an alias for mouse button 3.
    pub const MIDDLE: MouseButton = MouseButton::Button3;
}
