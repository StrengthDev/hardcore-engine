use std::collections::HashMap;
use std::ffi::OsStr;
use std::path::Path;
use std::ptr;
use thiserror::Error;
use tokio::fs::File;
use tokio::io::{AsyncReadExt, BufReader};

#[derive(Error, Debug)]
pub enum ShaderError {
    #[error(transparent)]
    IoError(#[from] std::io::Error),
    #[error("Invalid hardcore_sys::ShaderStage value")]
    InvalidStage,
    #[error("Could not infer shader stage from file extension \"{0}\"")]
    UnknownStage(String),
    #[cfg(feature = "shader-compilation")]
    #[error("Failed to acquire glslang compiler")]
    NoCompiler,
    #[cfg(feature = "shader-compilation")]
    #[error(transparent)]
    GLSLang(#[from] glslang::error::GlslangError),
}

#[derive(Copy, Clone, Debug)]
pub enum ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Mesh,
    TesselationControl,
    TesselationEvaluation,
    Geometry,
    Task,
    RayGeneration,
    RayIntersection,
    RayAnyHit,
    RayClosestHit,
    RayMiss,
    RayCallable,
}

impl ShaderStage {
    pub fn extension(&self) -> &'static str {
        match self {
            ShaderStage::Vertex => "vert",
            ShaderStage::Fragment => "frag",
            ShaderStage::Compute => "comp",
            // ShaderStage::Mesh => "",
            ShaderStage::TesselationControl => "tesc",
            ShaderStage::TesselationEvaluation => "tese",
            ShaderStage::Geometry => "geom",
            // ShaderStage::Task => "",
            ShaderStage::RayGeneration => "rgen",
            ShaderStage::RayIntersection => "rint",
            ShaderStage::RayAnyHit => "rahit",
            ShaderStage::RayClosestHit => "rchit",
            ShaderStage::RayMiss => "rmiss",
            ShaderStage::RayCallable => "rcall",
            _ => "",
        }
    }
}

impl TryFrom<&Path> for ShaderStage {
    type Error = ShaderError;

    fn try_from(path: &Path) -> Result<Self, Self::Error> {
        let extension = if let Some(extension) = path.extension().and_then(OsStr::to_str) {
            if extension == "bin" {
                if let Some(stem) = path.file_stem().and_then(OsStr::to_str) {
                    stem.rfind('.').map(|i| &stem[i..])
                } else {
                    None
                }
            } else {
                Some(extension)
            }
        } else {
            None
        };

        if let Some(extension) = extension {
            // Hard coding all the possible stages here is a bit nasty, but there is no alternative
            // without adding additional dependencies.
            let stages = [
                ShaderStage::Vertex,
                ShaderStage::Fragment,
                ShaderStage::Compute,
                ShaderStage::Mesh,
                ShaderStage::TesselationControl,
                ShaderStage::TesselationEvaluation,
                ShaderStage::Geometry,
                ShaderStage::Task,
                ShaderStage::RayGeneration,
                ShaderStage::RayIntersection,
                ShaderStage::RayAnyHit,
                ShaderStage::RayClosestHit,
                ShaderStage::RayMiss,
                ShaderStage::RayCallable,
            ];

            let stage_map = {
                let mut map = HashMap::new();
                for stage in stages {
                    let ext_str = stage.extension();
                    if !ext_str.is_empty() {
                        map.insert(stage.extension(), stage);
                    }
                }
                map
            };

            stage_map
                .get(extension)
                .copied()
                .ok_or(ShaderError::UnknownStage(extension.to_string()))
        } else {
            Err(ShaderError::UnknownStage("".to_string()))
        }
    }
}

impl TryFrom<hardcore_sys::ShaderStage> for ShaderStage {
    type Error = ShaderError;

    fn try_from(value: hardcore_sys::ShaderStage) -> Result<Self, Self::Error> {
        match value {
            hardcore_sys::ShaderStage::VertexStage => Ok(ShaderStage::Vertex),
            hardcore_sys::ShaderStage::FragmentStage => Ok(ShaderStage::Fragment),
            hardcore_sys::ShaderStage::ComputeStage => Ok(ShaderStage::Compute),
            hardcore_sys::ShaderStage::MeshStage => Ok(ShaderStage::Mesh),
            hardcore_sys::ShaderStage::TesselationControlStage => {
                Ok(ShaderStage::TesselationControl)
            }
            hardcore_sys::ShaderStage::TesselationEvaluationStage => {
                Ok(ShaderStage::TesselationEvaluation)
            }
            hardcore_sys::ShaderStage::GeometryStage => Ok(ShaderStage::Geometry),
            hardcore_sys::ShaderStage::TaskStage => Ok(ShaderStage::Task),
            hardcore_sys::ShaderStage::RayGenerationStage => Ok(ShaderStage::RayGeneration),
            hardcore_sys::ShaderStage::RayIntersectionStage => Ok(ShaderStage::RayIntersection),
            hardcore_sys::ShaderStage::RayAnyHitStage => Ok(ShaderStage::RayAnyHit),
            hardcore_sys::ShaderStage::RayClosestHitStage => Ok(ShaderStage::RayClosestHit),
            hardcore_sys::ShaderStage::RayMissStage => Ok(ShaderStage::RayMiss),
            hardcore_sys::ShaderStage::RayCallableStage => Ok(ShaderStage::RayCallable),
            _ => Err(ShaderError::InvalidStage),
        }
    }
}

impl From<ShaderStage> for hardcore_sys::ShaderStage {
    fn from(value: ShaderStage) -> Self {
        match value {
            ShaderStage::Vertex => hardcore_sys::ShaderStage::VertexStage,
            ShaderStage::Fragment => hardcore_sys::ShaderStage::FragmentStage,
            ShaderStage::Compute => hardcore_sys::ShaderStage::ComputeStage,
            ShaderStage::Mesh => hardcore_sys::ShaderStage::MeshStage,
            ShaderStage::TesselationControl => hardcore_sys::ShaderStage::TesselationControlStage,
            ShaderStage::TesselationEvaluation => {
                hardcore_sys::ShaderStage::TesselationEvaluationStage
            }
            ShaderStage::Geometry => hardcore_sys::ShaderStage::GeometryStage,
            ShaderStage::Task => hardcore_sys::ShaderStage::TaskStage,
            ShaderStage::RayGeneration => hardcore_sys::ShaderStage::RayGenerationStage,
            ShaderStage::RayIntersection => hardcore_sys::ShaderStage::RayIntersectionStage,
            ShaderStage::RayAnyHit => hardcore_sys::ShaderStage::RayAnyHitStage,
            ShaderStage::RayClosestHit => hardcore_sys::ShaderStage::RayClosestHitStage,
            ShaderStage::RayMiss => hardcore_sys::ShaderStage::RayMissStage,
            ShaderStage::RayCallable => hardcore_sys::ShaderStage::RayCallableStage,
        }
    }
}

pub struct Shader {
    inner: hardcore_sys::Shader,
    stage: ShaderStage,
}

unsafe impl Send for Shader {}

impl Shader {
    pub fn try_from_bytecode(bytecode: &[u32], stage: ShaderStage) -> Result<Shader, ShaderError> {
        let inner =
            unsafe { hardcore_sys::create_shader(bytecode.as_ptr(), bytecode.len(), stage.into()) };
        if inner.inner.is_null() {
            todo!()
        }
        Ok(Shader { inner, stage })
    }

    pub async fn try_from_binary_file<P: AsRef<Path>>(
        path: P,
        stage_hint: Option<ShaderStage>,
    ) -> Result<Shader, ShaderError> {
        let stage = if let Some(stage) = stage_hint {
            stage
        } else {
            ShaderStage::try_from(path.as_ref())?
        };

        let file = File::open(path).await?;
        let mut bytecode = if let Ok(meta) = file.metadata().await {
            Vec::with_capacity(meta.len() as usize / size_of::<u32>())
        } else {
            vec![]
        };

        let mut reader = BufReader::new(file);

        while let Ok(word) = reader.read_u32().await {
            bytecode.push(word);
        }

        Shader::try_from_bytecode(&bytecode, stage)
    }

    pub fn stage(&self) -> ShaderStage {
        self.stage
    }
}

impl Drop for Shader {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_shader(ptr::addr_of_mut!(self.inner)) }
    }
}

#[cfg(feature = "shader-compilation")]
mod shader_compilation {
    use crate::render::vulkan_version;
    use crate::shader::{Shader, ShaderError, ShaderStage};
    use crate::Version;
    use std::path::Path;
    use tokio::fs::File;
    use tokio::io::AsyncReadExt;
    use tracing::trace;

    impl From<glslang::ShaderStage> for ShaderStage {
        fn from(value: glslang::ShaderStage) -> Self {
            match value {
                glslang::ShaderStage::Vertex => ShaderStage::Vertex,
                glslang::ShaderStage::Fragment => ShaderStage::Fragment,
                glslang::ShaderStage::Compute => ShaderStage::Compute,
                glslang::ShaderStage::Mesh => ShaderStage::Mesh,
                glslang::ShaderStage::TesselationControl => ShaderStage::TesselationControl,
                glslang::ShaderStage::TesselationEvaluation => ShaderStage::TesselationEvaluation,
                glslang::ShaderStage::Geometry => ShaderStage::Geometry,
                glslang::ShaderStage::Task => ShaderStage::Task,
                glslang::ShaderStage::RayGeneration => ShaderStage::RayGeneration,
                glslang::ShaderStage::Intersect => ShaderStage::RayIntersection,
                glslang::ShaderStage::AnyHit => ShaderStage::RayAnyHit,
                glslang::ShaderStage::ClosestHit => ShaderStage::RayClosestHit,
                glslang::ShaderStage::Miss => ShaderStage::RayMiss,
                glslang::ShaderStage::Callable => ShaderStage::RayCallable,
            }
        }
    }

    impl From<ShaderStage> for glslang::ShaderStage {
        fn from(value: ShaderStage) -> Self {
            match value {
                ShaderStage::Vertex => glslang::ShaderStage::Vertex,
                ShaderStage::Fragment => glslang::ShaderStage::Fragment,
                ShaderStage::Compute => glslang::ShaderStage::Compute,
                ShaderStage::Mesh => glslang::ShaderStage::Mesh,
                ShaderStage::TesselationControl => glslang::ShaderStage::TesselationControl,
                ShaderStage::TesselationEvaluation => glslang::ShaderStage::TesselationEvaluation,
                ShaderStage::Geometry => glslang::ShaderStage::Geometry,
                ShaderStage::Task => glslang::ShaderStage::Task,
                ShaderStage::RayGeneration => glslang::ShaderStage::RayGeneration,
                ShaderStage::RayIntersection => glslang::ShaderStage::Intersect,
                ShaderStage::RayAnyHit => glslang::ShaderStage::AnyHit,
                ShaderStage::RayClosestHit => glslang::ShaderStage::ClosestHit,
                ShaderStage::RayMiss => glslang::ShaderStage::Miss,
                ShaderStage::RayCallable => glslang::ShaderStage::Callable,
            }
        }
    }

    /// A [SPIR-V] version.
    ///
    /// [SPIR-V]: https://registry.khronos.org/SPIR-V/
    #[derive(Copy, Clone, Debug, Default)]
    pub enum SpirvVersion {
        /// SPIR-V version `1.0`.
        V1_0,

        /// SPIR-V version `1.1`.
        V1_1,

        /// SPIR-V version `1.2`.
        V1_2,

        /// SPIR-V version `1.3`.
        V1_3,

        /// SPIR-V version `1.4`.
        V1_4,

        /// SPIR-V version `1.5`.
        V1_5,

        /// SPIR-V version `1.6`.
        #[default]
        V1_6,
    }

    impl From<SpirvVersion> for glslang::SpirvVersion {
        fn from(value: SpirvVersion) -> Self {
            match value {
                SpirvVersion::V1_0 => glslang::SpirvVersion::SPIRV1_0,
                SpirvVersion::V1_1 => glslang::SpirvVersion::SPIRV1_1,
                SpirvVersion::V1_2 => glslang::SpirvVersion::SPIRV1_2,
                SpirvVersion::V1_3 => glslang::SpirvVersion::SPIRV1_3,
                SpirvVersion::V1_4 => glslang::SpirvVersion::SPIRV1_4,
                SpirvVersion::V1_5 => glslang::SpirvVersion::SPIRV1_5,
                SpirvVersion::V1_6 => glslang::SpirvVersion::SPIRV1_6,
            }
        }
    }

    impl Shader {
        pub fn compile(
            source: &str,
            stage: ShaderStage,
            spirv_version: SpirvVersion,
        ) -> Result<Vec<u32>, ShaderError> {
            trace!("Compiling \"{stage:?}\" shader source to SPIR-V..");

            use glslang::{
                Compiler, CompilerOptions, ShaderInput, ShaderSource, Target, VulkanVersion,
            };

            let compiler = Compiler::acquire().ok_or(ShaderError::NoCompiler)?;
            let vulkan = match vulkan_version() {
                Version {
                    major: 1,
                    minor,
                    patch: 0,
                } => match minor {
                    0 => VulkanVersion::Vulkan1_0,
                    1 => VulkanVersion::Vulkan1_1,
                    2 => VulkanVersion::Vulkan1_2,
                    3 => VulkanVersion::Vulkan1_3,
                    _ => unreachable!("All Vulkan versions must be handled"),
                },
                Version { .. } => unreachable!("All Vulkan versions must be handled"),
            };
            let options = CompilerOptions {
                target: Target::Vulkan {
                    version: vulkan,
                    spirv_version: spirv_version.into(),
                },
                ..Default::default()
            };
            let source = ShaderSource::from(source);

            let input = ShaderInput::new(&source, stage.into(), &options, None, None)?;
            let shader = glslang::Shader::new(compiler, input)?;

            Ok(shader.compile()?)
        }

        pub fn try_from_source(
            source: &str,
            stage: ShaderStage,
            spirv_version: SpirvVersion,
        ) -> Result<Shader, ShaderError> {
            Shader::try_from_bytecode(
                Shader::compile(source, stage, spirv_version)?.as_slice(),
                stage,
            )
        }

        pub async fn try_from_source_file<P: AsRef<Path>>(
            path: P,
            stage_hint: Option<ShaderStage>,
            spirv_version: SpirvVersion,
        ) -> Result<Shader, ShaderError> {
            let stage = if let Some(stage) = stage_hint {
                stage
            } else {
                ShaderStage::try_from(path.as_ref())?
            };

            let mut file = File::open(path).await?;
            let mut source = String::new();
            file.read_to_string(&mut source).await?;

            Shader::try_from_source(&source, stage, spirv_version)
        }
    }
}

#[cfg(feature = "shader-compilation")]
pub use shader_compilation::*;
