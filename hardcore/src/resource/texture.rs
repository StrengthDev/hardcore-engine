use crate::handle::Handle;
use crate::Error;

pub use hardcore_sys::{
    TextureBlockSize, TextureComponentFormat, TextureCompression, TextureNumericFormat,
    TextureSampleCount,
};

use core::num::NonZeroU32;
use core::ops::{
    Bound, Range, RangeBounds, RangeFrom, RangeFull, RangeInclusive, RangeTo, RangeToInclusive,
};
use hardcore_sys::{
    TextureDimensions1D, TextureDimensions2D, TextureDimensions3D, TextureDimensionsX,
    TextureFormatCompressed, TextureFormatStandard, TextureFormatType, TextureFormatUnion,
    TextureType, TextureViewParams,
};

#[derive(Debug, Copy, Clone)]
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

#[derive(Debug, Copy, Clone)]
enum TextureFormatImpl {
    Standard {
        component_format: TextureComponentFormat,
        numeric_format: TextureNumericFormat,
    },
    Compressed {
        compression: TextureCompression,
        numeric_format: TextureNumericFormat,
        block_size: TextureBlockSize,
    },
}

impl TextureFormatImpl {
    fn validate(&self) -> bool {
        let c_format = (*self).into();
        unsafe { hardcore_sys::validate_texture_format(std::ptr::addr_of!(c_format)) }
    }
}

impl From<TextureFormatImpl> for hardcore_sys::TextureFormat {
    fn from(value: TextureFormatImpl) -> Self {
        match value {
            TextureFormatImpl::Standard {
                component_format,
                numeric_format,
            } => Self {
                format: TextureFormatUnion {
                    standard: TextureFormatStandard {
                        component_format,
                        numeric_format,
                    },
                },
                type_: TextureFormatType::Standard,
            },
            TextureFormatImpl::Compressed {
                compression,
                numeric_format,
                block_size,
            } => Self {
                format: TextureFormatUnion {
                    compressed: TextureFormatCompressed {
                        compression,
                        numeric_format,
                        block_size,
                    },
                },
                type_: TextureFormatType::Compressed,
            },
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub struct TextureFormat(TextureFormatImpl);

impl TextureFormat {
    pub fn standard(
        component_format: TextureComponentFormat,
        numeric_format: TextureNumericFormat,
    ) -> Result<Self, Error> {
        let format = TextureFormatImpl::Standard {
            component_format,
            numeric_format,
        };

        if !format.validate() {
            return Err(Error::FormatDoesNotExist);
        }

        Ok(TextureFormat(format))
    }

    pub fn compressed(
        compression: TextureCompression,
        numeric_format: TextureNumericFormat,
        block_size: TextureBlockSize,
    ) -> Result<Self, Error> {
        let format = TextureFormatImpl::Compressed {
            compression,
            numeric_format,
            block_size,
        };

        if !format.validate() {
            return Err(Error::FormatDoesNotExist);
        }

        Ok(TextureFormat(format))
    }
}

#[derive(Debug, Copy, Clone)]
enum ViewRangeValue {
    Unbounded,
    Bounded {
        from: u32,
        to: u32, // exclusive
    },
    BoundFrom(u32),
    BoundTo(u32), // exclusive
}

impl ViewRangeValue {
    fn get_offset_and_count(&self, count: u32) -> Result<(u32, u32), Error> {
        let ret = match self {
            ViewRangeValue::Unbounded => Some((0, count)),
            ViewRangeValue::Bounded { from, to } => {
                if *to <= count {
                    None
                } else {
                    Some((*from, to - from))
                }
            }
            ViewRangeValue::BoundFrom(from) => {
                if *from < count {
                    None
                } else {
                    Some((*from, count - from))
                }
            }
            ViewRangeValue::BoundTo(to) => {
                if *to <= count {
                    None
                } else {
                    Some((0, *to))
                }
            }
        };

        if let Some(value) = ret {
            Ok(value)
        } else {
            Err(Error::InvalidParams("Out of bounds range".to_string()))
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub struct ViewRange(ViewRangeValue);

impl From<ViewRangeValue> for ViewRange {
    fn from(value: ViewRangeValue) -> Self {
        ViewRange(value)
    }
}

// dumb work around for compiler "bug": https://github.com/rust-lang/rust/issues/50133

macro_rules! impl_try_from_view_range {
    ($($t:ty),*) => {
        $(

impl TryFrom<$t> for ViewRange {
    type Error = Error;

    fn try_from(value: $t) -> Result<Self, Self::Error> {
        let start_bound = match value.start_bound() {
            Bound::Included(start) => Some(*start),
            Bound::Excluded(start) => Some(*start + 1),
            Bound::Unbounded => None,
        };

        let end_bound = match value.end_bound() {
            Bound::Included(end) => Some(*end + 1),
            Bound::Excluded(end) => Some(*end),
            Bound::Unbounded => None,
        };

        let range = match (start_bound, end_bound) {
            (None, None) => ViewRangeValue::Unbounded,
            (Some(from), Some(to)) => {
                if from >= to {
                    return Err(Error::InvalidParams("Range cannot be empty".to_string()));
                }

                ViewRangeValue::Bounded { from, to }
            }
            (Some(from), None) => ViewRangeValue::BoundFrom(from),
            (None, Some(to)) => ViewRangeValue::BoundTo(to),
        };

        Ok(range.into())
    }
}

        )*
    };
}

impl_try_from_view_range! { Range<u32>, RangeFrom<u32>, RangeFull, RangeInclusive<u32>, RangeTo<u32>, RangeToInclusive<u32> }

impl TryFrom<u32> for ViewRange {
    type Error = Error;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        Ok(ViewRangeValue::Bounded {
            from: value,
            to: value + 1,
        }
        .into())
    }
}

#[derive(Debug, Copy, Clone)]
pub struct TextureView {
    layer_range: ViewRange,
    mip_level_range: ViewRange,
    cube: bool,
}

impl TextureView {
    pub fn full(cube: bool) -> TextureView {
        TextureView {
            layer_range: ViewRangeValue::Unbounded.into(),
            mip_level_range: ViewRangeValue::Unbounded.into(),
            cube,
        }
    }

    pub fn partial(
        layers: impl TryInto<ViewRange, Error = Error>,
        mip_levels: impl TryInto<ViewRange, Error = Error>,
        cube: bool,
    ) -> Result<TextureView, Error> {
        Ok(TextureView {
            layer_range: layers.try_into()?,
            mip_level_range: mip_levels.try_into()?,
            cube,
        })
    }

    fn as_view_params(
        &self,
        layer_count: NonZeroU32,
        mip_level_count: NonZeroU32,
    ) -> Result<TextureViewParams, Error> {
        let (base_layer, layer_count) =
            self.layer_range.0.get_offset_and_count(layer_count.get())?;
        let (base_mip_level, mip_level_count) = self
            .mip_level_range
            .0
            .get_offset_and_count(mip_level_count.get())?;

        Ok(TextureViewParams {
            base_layer,
            layer_count,
            base_mip_level,
            mip_level_count,
            cube: self.cube,
        })
    }
}

pub struct TextureBase<'s> {
    handle: Handle<'s, hardcore_sys::Texture>,
    layer_count: NonZeroU32,
    mip_level_count: NonZeroU32,
    cube_compatible: bool,
}

impl<'s> TextureBase<'s> {
    fn new(
        device: u32,
        dimensions: TextureDimensions,
        format: TextureFormat,
        mip_level_count: NonZeroU32,
        sample_count: TextureSampleCount,
        cube_compatible: bool,
        render_target: bool,
    ) -> Result<TextureBase<'s>, Error> {
        let mut handle = Default::default();
        unsafe {
            hardcore_sys::new_texture(
                &raw mut handle,
                device,
                dimensions.into(),
                format.0.into(),
                mip_level_count.get(),
                sample_count,
                render_target,
            )
            .into_std_result()?
        };

        let layer_count = match dimensions {
            TextureDimensions::Texture1D { layers, .. } => layers,
            TextureDimensions::Texture2D { layers, .. } => layers,
            TextureDimensions::Texture3D { .. } => unsafe { NonZeroU32::new_unchecked(1) },
        };

        Ok(TextureBase {
            handle: handle.into(),
            layer_count,
            mip_level_count,
            cube_compatible,
        })
    }
}

impl BasicTexture for TextureBase<'_> {
    fn handle(&self) -> hardcore_sys::Texture {
        self.handle.inner
    }

    fn c_view_params(&self, view: TextureView) -> Result<TextureViewParams, Error> {
        view.as_view_params(self.layer_count, self.mip_level_count)
    }
}

impl<'s> Drop for TextureBase<'s> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_texture(self.handle.mut_ptr()) }
    }
}

pub struct Texture<'s>(TextureBase<'s>);

impl<'s> Texture<'s> {
    pub(crate) fn new(
        device: u32,
        dimensions: TextureDimensions,
        format: TextureFormat,
        mip_level_count: NonZeroU32,
        sample_count: TextureSampleCount,
        cube_compatible: bool,
    ) -> Result<Texture<'s>, Error> {
        TextureBase::new(
            device,
            dimensions,
            format,
            mip_level_count,
            sample_count,
            cube_compatible,
            false,
        )
        .map(Texture)
    }
}

impl BasicTexture for Texture<'_> {
    fn handle(&self) -> hardcore_sys::Texture {
        self.0.handle()
    }

    fn c_view_params(&self, view: TextureView) -> Result<TextureViewParams, Error> {
        self.0.c_view_params(view)
    }
}

pub struct RenderTarget<'s>(TextureBase<'s>);

impl<'s> RenderTarget<'s> {
    pub(crate) fn new(
        device: u32,
        dimensions: TextureDimensions,
        format: TextureFormat,
        mip_level_count: NonZeroU32,
        sample_count: TextureSampleCount,
        cube_compatible: bool,
    ) -> Result<RenderTarget<'s>, Error> {
        TextureBase::new(
            device,
            dimensions,
            format,
            mip_level_count,
            sample_count,
            cube_compatible,
            true,
        )
        .map(RenderTarget)
    }
}

impl BasicTexture for RenderTarget<'_> {
    fn handle(&self) -> hardcore_sys::Texture {
        self.0.handle()
    }

    fn c_view_params(&self, view: TextureView) -> Result<TextureViewParams, Error> {
        self.0.c_view_params(view)
    }
}

pub(crate) trait BasicTexture {
    fn handle(&self) -> hardcore_sys::Texture;

    fn c_view_params(&self, view: TextureView) -> Result<TextureViewParams, Error>;
}

pub(crate) trait InputAttachmentTexture: BasicTexture {}

pub struct DepthStencilTexture<'s> {
    texture: TextureBase<'s>,
}

impl BasicTexture for DepthStencilTexture<'_> {
    fn handle(&self) -> hardcore_sys::Texture {
        self.texture.handle.inner
    }

    fn c_view_params(&self, view: TextureView) -> Result<TextureViewParams, Error> {
        self.texture.c_view_params(view)
    }
}
