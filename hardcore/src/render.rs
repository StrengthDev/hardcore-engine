use crate::Version;

/// Get the version of Vulkan that is in use.
pub fn vulkan_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::VULKAN_VERSION.major },
        minor: unsafe { hardcore_sys::VULKAN_VERSION.minor },
        patch: unsafe { hardcore_sys::VULKAN_VERSION.patch },
    }
}
