use crate::handle::Handle;
use crate::ops::{as_dependency, seal, Dependency, Predicate, Schedule};
use crate::resource::{
    BasicTexture, DepthStencilTexture, InputAttachmentTexture, RenderTarget, TextureView,
};
use crate::Error;

use std::collections::HashMap;

pub struct InputAttachment<'t> {
    pub attachment: &'t dyn InputAttachmentTexture,
    pub view: TextureView,
    pub dependency: Option<&'t dyn Dependency>,
}

pub struct OutputAttachment<'t, 's> {
    pub attachment: &'t RenderTarget<'s>,
    pub view: TextureView,
    pub dependency: Option<&'t dyn Dependency>,
}

pub struct DepthStencilAttachment<'t, 's> {
    pub attachment: &'t DepthStencilTexture<'s>,
    pub view: TextureView,
    pub dependency: Option<&'t dyn Dependency>,
}

pub struct SubpassInfo {
    inputs: Vec<(
        hardcore_sys::Texture,
        hardcore_sys::TextureViewParams,
        u32,
        hardcore_sys::Dependency,
    )>,
    outputs: Vec<(
        hardcore_sys::Texture,
        hardcore_sys::TextureViewParams,
        u32,
        hardcore_sys::Dependency,
    )>,
    depth_stencil: Option<(
        hardcore_sys::Texture,
        hardcore_sys::TextureViewParams,
        hardcore_sys::Dependency,
    )>,
    device: u32,
}

impl SubpassInfo {
    pub fn new(
        inputs: &HashMap<u32, InputAttachment>,
        outputs: &HashMap<u32, OutputAttachment>,
        depth_stencil: Option<DepthStencilAttachment>,
    ) -> Result<SubpassInfo, Error> {
        if outputs.is_empty() {
            return Err(Error::InvalidParams(
                "Subpass must have at least one output".to_string(),
            ));
        }

        let mut c_inputs = Vec::with_capacity(inputs.len());
        for (index, input) in inputs {
            c_inputs.push((
                input.attachment.handle(),
                input.attachment.c_view_params(input.view)?,
                *index,
                as_dependency(input.dependency),
            ))
        }

        let mut c_outputs = Vec::with_capacity(outputs.len());
        for (location, output) in outputs {
            c_outputs.push((
                output.attachment.handle(),
                output.attachment.c_view_params(output.view)?,
                *location,
                as_dependency(output.dependency),
            ))
        }

        let depth_stencil = match depth_stencil {
            None => None,
            Some(value) => Some((
                value.attachment.handle(),
                value.attachment.c_view_params(value.view)?,
                as_dependency(value.dependency),
            )),
        };

        let device = c_outputs[0].0.device;
        for &(attachment, _, _, _) in c_inputs.iter().chain(&c_outputs) {
            if attachment.device != device {
                return Err(Error::InvalidParams(
                    "All sub-pass resources must be owned by the same device".to_string(),
                ));
            }
        }

        if let Some((texture, _, _)) = depth_stencil {
            if texture.device != device {
                return Err(Error::InvalidParams(
                    "All sub-pass resources must be owned by the same device".to_string(),
                ));
            }
        }

        Ok(SubpassInfo {
            inputs: c_inputs,
            outputs: c_outputs,
            depth_stencil,
            device,
        })
    }
}

struct SubpassData {
    inputs: Vec<hardcore_sys::SubpassInputAttachment>,
    outputs: Vec<hardcore_sys::SubpassOutputAttachment>,
    depth_stencil: Option<hardcore_sys::SubpassDepthStencilAttachment>,
}

pub struct RenderPass<'s> {
    pub(super) handle: Handle<'s, hardcore_sys::RenderPass>,
    predicate: Option<Predicate>,
}

impl<'s> RenderPass<'s> {
    pub(crate) fn new(
        subpasses: &[SubpassInfo],
        schedule: Schedule<impl Fn(usize) -> bool + 'static>,
    ) -> Result<RenderPass<'s>, Error> {
        if subpasses.is_empty() {
            return Err(Error::InvalidParams(
                "Render passes must have at least one subpass".to_string(),
            ));
        }

        let device = subpasses[0].device;
        for sub_pass in subpasses {
            if sub_pass.device != device {
                return Err(Error::InvalidParams(
                    "All render pass resources must be owned by the same device".to_string(),
                ));
            }
        }

        let mut handle = Default::default();

        let mut c_subpass_datas = Vec::with_capacity(subpasses.len());
        let mut c_subpasses = Vec::with_capacity(subpasses.len());

        for subpass in subpasses {
            c_subpass_datas.push(SubpassData {
                inputs: subpass
                    .inputs
                    .iter()
                    .map(move |(texture, view_params, index, dependency)| {
                        hardcore_sys::SubpassInputAttachment {
                            texture_id: texture.id,
                            texture_view_params: *view_params,
                            index: *index,
                            dependency: *dependency,
                        }
                    })
                    .collect(),
                outputs: subpass
                    .outputs
                    .iter()
                    .map(move |(texture, view_params, location, dependency)| {
                        hardcore_sys::SubpassOutputAttachment {
                            texture_id: texture.id,
                            texture_view_params: *view_params,
                            location: *location,
                            dependency: *dependency,
                        }
                    })
                    .collect(),
                depth_stencil: subpass.depth_stencil.map(
                    move |(texture, view_params, dependency)| {
                        hardcore_sys::SubpassDepthStencilAttachment {
                            texture_id: texture.id,
                            texture_view_params: view_params,
                            dependency,
                        }
                    },
                ),
            });

            let subpass_data = c_subpass_datas.last().unwrap();
            c_subpasses.push(hardcore_sys::Subpass {
                inputs: subpass_data.inputs.as_ptr(),
                input_count: subpass_data.inputs.len() as u32,
                outputs: subpass_data.outputs.as_ptr(),
                output_count: subpass_data.outputs.len() as u32,
                depth_stencil_attachment: subpass_data
                    .depth_stencil
                    .as_ref()
                    .map_or_else(std::ptr::null, move |attachment| attachment),
            })
        }

        let predicate: Option<Predicate> = schedule.into();
        let (predicate_fn, user_data) = Predicate::c_ptrs(&predicate);

        unsafe {
            hardcore_sys::new_render_pass(
                &raw mut handle,
                device,
                c_subpasses.as_ptr(),
                c_subpasses.len() as u32,
                predicate_fn,
                user_data,
            )
            .into_std_result()?
        };

        Ok(RenderPass {
            handle: handle.into(),
            predicate,
        })
    }
}

impl Drop for RenderPass<'_> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_render_pass(self.handle.mut_ptr()) }
    }
}

impl seal::Seal for RenderPass<'_> {}

impl Dependency for RenderPass<'_> {
    fn as_dependency(&self) -> hardcore_sys::Dependency {
        hardcore_sys::Dependency {
            handle: self.handle.ptr() as *const core::ffi::c_void,
            type_: hardcore_sys::DependencyType::RenderPass,
        }
    }
}
