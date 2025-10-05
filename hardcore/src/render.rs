use crate::Version;

/// The [Vulkan] API version that is used.
///
/// [Vulkan]: https://www.vulkan.org/
pub fn vulkan_api_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::VULKAN_API_VERSION.major },
        minor: unsafe { hardcore_sys::VULKAN_API_VERSION.minor },
        patch: unsafe { hardcore_sys::VULKAN_API_VERSION.patch },
    }
}

/// The [Vulkan headers][vk_headers] version that was compiled.
///
/// [vk_headers]: https://github.com/KhronosGroup/Vulkan-Headers
pub fn vulkan_header_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::VULKAN_HEADERS_VERSION.major },
        minor: unsafe { hardcore_sys::VULKAN_HEADERS_VERSION.minor },
        patch: unsafe { hardcore_sys::VULKAN_HEADERS_VERSION.patch },
    }
}

/// The [Volk] header version that was compiled.
///
/// [Volk]: https://github.com/zeux/volk
pub fn volk_header_version() -> u32 {
    unsafe { hardcore_sys::VOLK_HEADER_VERSION }
}
