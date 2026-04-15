use crate::handle::Handle;
use crate::ops::{Predicate, Schedule};
use crate::Error;

pub struct RasterPipeline<'s> {
    handle: Handle<'s, hardcore_sys::RasterPipeline>,
    predicate: Option<Predicate>,
}

impl<'s> RasterPipeline<'s> {
    pub(crate) fn new(
        device: u32,
        schedule: Schedule<impl Fn(usize) -> bool + 'static>,
    ) -> Result<RasterPipeline<'s>, Error> {
        let mut handle = Default::default();

        let predicate: Option<Predicate> = schedule.into();
        let (predicate_fn, user_data) = Predicate::c_ptrs(&predicate);

        unsafe {
            hardcore_sys::new_raster_pipeline(&raw mut handle, device, predicate_fn, user_data)
                .into_std_result()?
        };

        Ok(RasterPipeline {
            handle: handle.into(),
            predicate,
        })
    }
}

impl Drop for RasterPipeline<'_> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_raster_pipeline(self.handle.mut_ptr()) }
    }
}
