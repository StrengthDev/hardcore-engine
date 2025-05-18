use crate::Version;

/// Get the Vulkan API version that is used.
pub fn vulkan_api_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::VULKAN_API_VERSION.major },
        minor: unsafe { hardcore_sys::VULKAN_API_VERSION.minor },
        patch: unsafe { hardcore_sys::VULKAN_API_VERSION.patch },
    }
}

/// Get the Vulkan header version that was compiled.
pub fn vulkan_header_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::VULKAN_HEADER_VERSION.major },
        minor: unsafe { hardcore_sys::VULKAN_HEADER_VERSION.minor },
        patch: unsafe { hardcore_sys::VULKAN_HEADER_VERSION.patch },
    }
}

/// Get the Volk header version that was compiled.
pub fn volk_header_version() -> u32 {
    unsafe { hardcore_sys::VOLK_HEADER_VERSION }
}
