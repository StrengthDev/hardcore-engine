use crate::color::{Color, WHITE};
use crate::handle::Handle;
use crate::shader::Shader;
use crate::Error;

pub use hardcore_sys::{
    BlendFactor, BlendMode, BlendOp, PrimitiveTopology, TriangleCullMode, TriangleFrontFace,
};

pub enum PolygonMode {
    Fill,
    Line { width: f32 },
    Point,
}

struct PolygonInfo {
    mode: hardcore_sys::PolygonMode,
    line_width: f32,
}

impl PolygonMode {
    fn polygon_info(&self) -> PolygonInfo {
        match self {
            PolygonMode::Fill => PolygonInfo {
                mode: hardcore_sys::PolygonMode::Fill,
                line_width: 0.0,
            },
            PolygonMode::Line { width } => PolygonInfo {
                mode: hardcore_sys::PolygonMode::Line,
                line_width: *width,
            },
            PolygonMode::Point => PolygonInfo {
                mode: hardcore_sys::PolygonMode::Point,
                line_width: 0.0,
            },
        }
    }
}

pub struct DepthBias {
    pub constant_factor: f32,
    pub clamp: f32,
    pub slope_factor: f32,
}

pub struct RasterPipelineInfo {
    pub primitive_topology: PrimitiveTopology,
    pub discard_primitives: bool,
    pub polygon_mode: PolygonMode,
    pub triangle_cull_mode: TriangleCullMode,
    pub triangle_front_face: TriangleFrontFace,
    pub depth_bias: Option<DepthBias>,
    pub blend_mode: BlendMode,
    pub blend_constants: Color,
}

impl Default for RasterPipelineInfo {
    fn default() -> Self {
        RasterPipelineInfo {
            primitive_topology: PrimitiveTopology::TriangleList,
            discard_primitives: false,
            polygon_mode: PolygonMode::Fill,
            triangle_cull_mode: TriangleCullMode::Back,
            triangle_front_face: TriangleFrontFace::CounterClockwise,
            depth_bias: None,
            blend_mode: unsafe { hardcore_sys::ALPHA_BLENDING_BLEND_MODE },
            blend_constants: WHITE,
        }
    }
}

impl From<RasterPipelineInfo> for hardcore_sys::RasterPipelineInfo {
    fn from(value: RasterPipelineInfo) -> Self {
        let polygon_info = value.polygon_mode.polygon_info();

        let (constant_factor, clamp, slope_factor) = if let Some(ref depth_bias) = value.depth_bias
        {
            (
                depth_bias.constant_factor,
                depth_bias.clamp,
                depth_bias.slope_factor,
            )
        } else {
            (0.0, 0.0, 0.0)
        };

        hardcore_sys::RasterPipelineInfo {
            primitive_topology: value.primitive_topology,
            discard_primitives: value.discard_primitives,
            polygon_mode: polygon_info.mode,
            triangle_cull_mode: value.triangle_cull_mode,
            triangle_front_face: value.triangle_front_face,
            line_width: polygon_info.line_width,
            depth_bias: value.depth_bias.is_some(),
            depth_bias_constant_factor: constant_factor,
            depth_bias_clamp: clamp,
            depth_bias_slope_factor: slope_factor,
            blend_mode: value.blend_mode,
            blend_constants: value.blend_constants.into(),
        }
    }
}

pub struct RasterPipeline<'s> {
    pub(super) handle: Handle<'s, hardcore_sys::RasterPipeline>,
}

impl<'s> RasterPipeline<'s> {
    pub(crate) fn new(
        device: u32,
        shaders: &[&Shader],
        info: RasterPipelineInfo,
    ) -> Result<RasterPipeline<'s>, Error> {
        let mut handle = Default::default();

        let shader_vec: Vec<_> = shaders.iter().map(move |s| s.inner).collect();

        unsafe {
            hardcore_sys::new_raster_pipeline(
                &raw mut handle,
                device,
                shader_vec.as_ptr(),
                shaders.len() as u32,
                info.into(),
            )
            .into_std_result()?
        };

        Ok(RasterPipeline {
            handle: handle.into(),
        })
    }
}

impl Drop for RasterPipeline<'_> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_raster_pipeline(self.handle.mut_ptr()) }
    }
}
