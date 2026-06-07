use crate::handle::Handle;
use crate::ops::raster_pipeline::RasterPipeline;
use crate::ops::render_pass::RenderPass;
use crate::Error;

pub struct Draw<'s> {
    handle: Handle<'s, hardcore_sys::Draw>,
}

impl<'s> Draw<'s> {
    pub(crate) fn new(
        render_pass: &RenderPass<'s>,
        subpass: u32,
        pipeline: &RasterPipeline<'s>,
        vertex_count: u32,
        instance_count: u32,
    ) -> Result<Draw<'s>, Error> {
        if render_pass.handle.inner.device != pipeline.handle.inner.device {
            return Err(Error::InvalidParams(
                "Render pass and pipeline do no belong to the same device".to_string(),
            ));
        }

        let mut handle = Default::default();

        unsafe {
            hardcore_sys::new_draw(
                &raw mut handle,
                render_pass.handle.ptr(),
                subpass,
                pipeline.handle.ptr(),
                vertex_count,
                instance_count,
            )
            .into_std_result()?;
        }

        Ok(Draw {
            handle: handle.into(),
        })
    }

    // pub fn set_push_constants(&self) -> Result<(), Error> {
    //     unsafe { hardcore_sys::set_draw_push_constants(self.handle.ptr()) }
    // }
}

impl Drop for Draw<'_> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_draw(self.handle.mut_ptr()) }
    }
}
