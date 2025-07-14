use core::num::NonZeroU32;
use hardcore_sys::{
    TextureDimensions1D, TextureDimensions2D, TextureDimensions3D, TextureDimensionsX,
    TextureFormatID, TextureType,
};
use std::ptr;
use thiserror::Error;

pub use hardcore_sys::{
    TextureBlockSize, TextureComponentFormat, TextureCompression, TextureNumericFormat,
    TextureSampleCount,
};

#[derive(Error, Debug)]
pub enum TextureError {
    /// An error has occurred withing the system crate.
    #[error(transparent)]
    SystemError(#[from] hardcore_sys::Error),

    #[error("The specified texture format does not exist")]
    FormatDoesNotExist,
}

pub enum TextureDimensions {
    Texture1D {
        width: NonZeroU32,
        layers: NonZeroU32,
    },
    Texture2D {
        width: NonZeroU32,
        height: NonZeroU32,
        layers: NonZeroU32,
    },
    Texture3D {
        width: NonZeroU32,
        height: NonZeroU32,
        depth: NonZeroU32,
    },
}

impl From<TextureDimensions> for hardcore_sys::TextureDimensions {
    fn from(value: TextureDimensions) -> Self {
        match value {
            TextureDimensions::Texture1D { width, layers } => hardcore_sys::TextureDimensions {
                dims: TextureDimensionsX {
                    dims1D: TextureDimensions1D {
                        width: width.get(),
                        layers: layers.get(),
                    },
                },
                texture_type: TextureType::Texture1D,
            },
            TextureDimensions::Texture2D {
                width,
                height,
                layers,
            } => hardcore_sys::TextureDimensions {
                dims: TextureDimensionsX {
                    dims2D: TextureDimensions2D {
                        width: width.get(),
                        height: height.get(),
                        layers: layers.get(),
                    },
                },
                texture_type: TextureType::Texture2D,
            },
            TextureDimensions::Texture3D {
                width,
                height,
                depth,
            } => hardcore_sys::TextureDimensions {
                dims: TextureDimensionsX {
                    dims3D: TextureDimensions3D {
                        width: width.get(),
                        height: height.get(),
                        depth: depth.get(),
                    },
                },
                texture_type: TextureType::Texture3D,
            },
        }
    }
}

#[derive(Debug)]
pub struct TextureFormat(TextureFormatID);

impl TextureFormat {
    pub fn standard(
        component_format: TextureComponentFormat,
        numeric_format: TextureNumericFormat,
    ) -> Result<Self, TextureError> {
        let format =
            unsafe { hardcore_sys::texture_format_id_standard(component_format, numeric_format) };

        if format == i32::MAX {
            return Err(TextureError::FormatDoesNotExist);
        }

        Ok(TextureFormat(format))
    }

    pub fn compressed(
        compression: TextureCompression,
        numeric_format: TextureNumericFormat,
        block_size: TextureBlockSize,
    ) -> Result<Self, TextureError> {
        let format = unsafe {
            hardcore_sys::texture_format_id_compressed(compression, numeric_format, block_size)
        };

        if format == i32::MAX {
            return Err(TextureError::FormatDoesNotExist);
        }

        Ok(TextureFormat(format))
    }
}

pub struct Texture {
    handle: hardcore_sys::Texture,
}

impl Texture {
    pub fn create(
        device: u32,
        dimensions: TextureDimensions,
        format_id: TextureFormat,
        mip_levels: u32,
        sample_count: TextureSampleCount,
    ) -> Result<Self, TextureError> {
        let mut handle = Default::default();
        unsafe {
            hardcore_sys::create_texture(
                ptr::addr_of_mut!(handle),
                device,
                dimensions.into(),
                format_id.0,
                mip_levels,
                sample_count,
            )
            .into_std_result()?
        };

        Ok(Texture { handle })
    }
}

impl Drop for Texture {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_texture(ptr::addr_of_mut!(self.handle)) }
    }
}
