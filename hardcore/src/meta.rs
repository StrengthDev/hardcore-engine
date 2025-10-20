use std::fmt::{Display, Formatter};

macro_rules! get_version {
    ($env_var:literal) => {
        if let Ok(value) = u32::from_str_radix(env!($env_var), 10) {
            value
        } else {
            0
        }
    };
}

/// Hardcore's version.
pub static VERSION: Version = Version {
    major: get_version!("CARGO_PKG_VERSION_MAJOR"),
    minor: get_version!("CARGO_PKG_VERSION_MINOR"),
    patch: get_version!("CARGO_PKG_VERSION_PATCH"),
};

/// A version value.
#[derive(Clone, Debug)]
pub struct Version {
    /// The major version.
    pub major: u32,

    /// The minor version.
    pub minor: u32,

    /// The patch version.
    pub patch: u32,
}

impl Display for Version {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_str(format!("v{}.{}.{}", self.major, self.minor, self.patch).as_str())
    }
}

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

/// The [GLFW] version that was compiled.
///
/// [GLFW]: https://www.glfw.org/
pub fn glfw_version() -> Version {
    Version {
        major: unsafe { hardcore_sys::GLFW_VERSION.major },
        minor: unsafe { hardcore_sys::GLFW_VERSION.minor },
        patch: unsafe { hardcore_sys::GLFW_VERSION.patch },
    }
}
