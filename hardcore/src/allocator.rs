use crate::device::Device;
use crate::Error;

use crate::ops::raster_commands::Draw;
use crate::ops::raster_pipeline::{RasterPipeline, RasterPipelineInfo};
use crate::ops::render_pass::{RenderPass, SubpassInfo};
use crate::ops::Schedule;
use crate::resource::{RenderTarget, Texture, TextureDimensions, TextureFormat};
use crate::shader::Shader;
use hardcore_sys::TextureSampleCount;
use std::num::{NonZeroU32, NonZeroU64};

pub(super) mod seal {
    pub trait Seal {}
}

pub trait Allocator<'a>: seal::Seal {
    /// Instantiate a new window.
    fn new_window(
        &'a self,
        device: &Device,
        width: u32,
        height: u32,
        pos_x: Option<i32>,
        pos_y: Option<i32>,
        name: &str,
    ) -> Result<crate::io::window::Window<'a>, Error> {
        crate::io::window::Window::new(
            device.io_caller.clone(),
            device.id,
            width,
            height,
            pos_x,
            pos_y,
            name,
        )
    }

    fn new_texture(
        &'a self,
        device: &Device,
        dimensions: TextureDimensions,
        format: TextureFormat,
        mip_level_count: NonZeroU32,
        sample_count: TextureSampleCount,
        cube_compatible: bool,
    ) -> Result<Texture<'a>, Error> {
        Texture::new(
            device.id,
            dimensions,
            format,
            mip_level_count,
            sample_count,
            cube_compatible,
        )
    }

    fn new_render_target(
        &'a self,
        device: &Device,
        dimensions: TextureDimensions,
        format: TextureFormat,
        mip_level_count: NonZeroU32,
        sample_count: TextureSampleCount,
        cube_compatible: bool,
    ) -> Result<RenderTarget<'a>, Error> {
        RenderTarget::new(
            device.id,
            dimensions,
            format,
            mip_level_count,
            sample_count,
            cube_compatible,
        )
    }

    fn new_render_pass(&'a self, subpasses: &[SubpassInfo]) -> Result<RenderPass<'a>, Error> {
        RenderPass::new(subpasses, Schedule::<fn(usize) -> bool>::EveryFrame)
    }

    fn new_raster_pipeline(
        &'a self,
        device: &Device,
        shaders: &[&Shader],
        info: RasterPipelineInfo,
    ) -> Result<RasterPipeline<'a>, Error> {
        RasterPipeline::new(device.id, shaders, info)
    }

    fn new_draw(
        &'a self,
        render_pass: &RenderPass<'a>,
        subpass: u32,
        pipeline: &RasterPipeline<'a>,
        vertex_count: u32,
        instance_count: u32,
    ) -> Result<Draw<'a>, Error> {
        Draw::new(render_pass, subpass, pipeline, vertex_count, instance_count)
    }

    fn new_vertex_buffer(
        &'a self,
        device: &Device,
        descriptor: &crate::resource::descriptor::Descriptor,
        count: NonZeroU64,
    ) -> Result<crate::resource::VertexBuffer<'a, false>, Error> {
        crate::resource::VertexBuffer::<'a, false>::new(device.id, descriptor, count)
    }
}
