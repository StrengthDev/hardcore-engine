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
    #[error("Failed to create new texture")]
    Initialisation,
    #[error("The specified texture format does not exist")]
    FormatDoesNotExist,
}

pub enum TextureDimensions {
    Texture1D {
        width: u32,
        layers: u32,
    },
    Texture2D {
        width: u32,
        height: u32,
        layers: u32,
    },
    Texture3D {
        width: u32,
        height: u32,
        depth: u32,
    },
}

impl From<TextureDimensions> for hardcore_sys::TextureDimensions {
    fn from(value: TextureDimensions) -> Self {
        match value {
            TextureDimensions::Texture1D { width, layers } => hardcore_sys::TextureDimensions {
                dims: TextureDimensionsX {
                    dims1D: TextureDimensions1D { width, layers },
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
                        width,
                        height,
                        layers,
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
                        width,
                        height,
                        depth,
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
        let handle = unsafe {
            hardcore_sys::create_texture(
                device,
                dimensions.into(),
                format_id.0,
                mip_levels,
                sample_count,
            )
        };

        if handle.size == 0 {
            return Err(TextureError::Initialisation);
        }

        Ok(Texture { handle })
    }
}

impl Drop for Texture {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_texture(ptr::addr_of_mut!(self.handle)) }
    }
}
