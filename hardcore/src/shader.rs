use crate::Error;
use std::collections::HashMap;
use std::ffi::OsStr;
use std::path::Path;
use tokio::fs::File;
use tokio::io::{AsyncReadExt, BufReader};

#[derive(Debug, Copy, Clone)]
pub struct ShaderStage(hardcore_sys::ShaderStage);

impl ShaderStage {
    pub fn extension(&self) -> &'static str {
        match self.0 {
            hardcore_sys::ShaderStage::Vertex => "vert",
            hardcore_sys::ShaderStage::Fragment => "frag",
            hardcore_sys::ShaderStage::Compute => "comp",
            // hardcore_sys::ShaderStage::Mesh => "",
            hardcore_sys::ShaderStage::TesselationControl => "tesc",
            hardcore_sys::ShaderStage::TesselationEvaluation => "tese",
            hardcore_sys::ShaderStage::Geometry => "geom",
            // hardcore_sys::ShaderStage::Task => "",
            hardcore_sys::ShaderStage::RayGeneration => "rgen",
            hardcore_sys::ShaderStage::RayIntersection => "rint",
            hardcore_sys::ShaderStage::RayAnyHit => "rahit",
            hardcore_sys::ShaderStage::RayClosestHit => "rchit",
            hardcore_sys::ShaderStage::RayMiss => "rmiss",
            hardcore_sys::ShaderStage::RayCallable => "rcall",
            _ => "",
        }
    }
}

impl From<hardcore_sys::ShaderStage> for ShaderStage {
    fn from(value: hardcore_sys::ShaderStage) -> Self {
        ShaderStage(value)
    }
}

impl From<ShaderStage> for hardcore_sys::ShaderStage {
    fn from(value: ShaderStage) -> Self {
        value.0
    }
}

impl TryFrom<&Path> for ShaderStage {
    type Error = Error;

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
                hardcore_sys::ShaderStage::Vertex,
                hardcore_sys::ShaderStage::Fragment,
                hardcore_sys::ShaderStage::Compute,
                hardcore_sys::ShaderStage::Mesh,
                hardcore_sys::ShaderStage::TesselationControl,
                hardcore_sys::ShaderStage::TesselationEvaluation,
                hardcore_sys::ShaderStage::Geometry,
                hardcore_sys::ShaderStage::Task,
                hardcore_sys::ShaderStage::RayGeneration,
                hardcore_sys::ShaderStage::RayIntersection,
                hardcore_sys::ShaderStage::RayAnyHit,
                hardcore_sys::ShaderStage::RayClosestHit,
                hardcore_sys::ShaderStage::RayMiss,
                hardcore_sys::ShaderStage::RayCallable,
            ];

            let stage_map = {
                let mut map = HashMap::new();
                for stage in stages {
                    let stage: ShaderStage = stage.into();
                    let ext_str = stage.extension();
                    if !ext_str.is_empty() {
                        map.insert(ext_str, stage);
                    }
                }
                map
            };

            stage_map
                .get(extension)
                .copied()
                .ok_or(Error::UnknownStage(extension.to_string()))
        } else {
            Err(Error::UnknownStage("".to_string()))
        }
    }
}

pub struct Shader {
    inner: hardcore_sys::Shader,
    stage: ShaderStage,
}

unsafe impl Send for Shader {}

impl Shader {
    pub fn try_from_bytecode(bytecode: &[u32], stage: ShaderStage) -> Result<Shader, Error> {
        let mut inner = Default::default();

        unsafe {
            hardcore_sys::create_shader(
                &raw mut inner,
                bytecode.as_ptr(),
                bytecode.len(),
                stage.into(),
            )
            .into_std_result()?
        };

        Ok(Shader { inner, stage })
    }

    pub async fn try_from_binary_file<P: AsRef<Path>>(
        path: P,
        stage_hint: Option<ShaderStage>,
    ) -> Result<Shader, Error> {
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
        unsafe { hardcore_sys::destroy_shader(&raw mut self.inner) }
    }
}

#[cfg(feature = "shader-compilation")]
mod compilation {
    use crate::meta::vulkan_api_version;
    use crate::shader::{Shader, Error, ShaderStage};
    use crate::Version;
    use std::path::Path;
    use tokio::fs::File;
    use tokio::io::AsyncReadExt;
    use tracing::trace;

    impl From<glslang::ShaderStage> for ShaderStage {
        fn from(value: glslang::ShaderStage) -> Self {
            let stage = match value {
                glslang::ShaderStage::Vertex => hardcore_sys::ShaderStage::Vertex,
                glslang::ShaderStage::Fragment => hardcore_sys::ShaderStage::Fragment,
                glslang::ShaderStage::Compute => hardcore_sys::ShaderStage::Compute,
                glslang::ShaderStage::Mesh => hardcore_sys::ShaderStage::Mesh,
                glslang::ShaderStage::TesselationControl => {
                    hardcore_sys::ShaderStage::TesselationControl
                }
                glslang::ShaderStage::TesselationEvaluation => {
                    hardcore_sys::ShaderStage::TesselationEvaluation
                }
                glslang::ShaderStage::Geometry => hardcore_sys::ShaderStage::Geometry,
                glslang::ShaderStage::Task => hardcore_sys::ShaderStage::Task,
                glslang::ShaderStage::RayGeneration => hardcore_sys::ShaderStage::RayGeneration,
                glslang::ShaderStage::Intersect => hardcore_sys::ShaderStage::RayIntersection,
                glslang::ShaderStage::AnyHit => hardcore_sys::ShaderStage::RayAnyHit,
                glslang::ShaderStage::ClosestHit => hardcore_sys::ShaderStage::RayClosestHit,
                glslang::ShaderStage::Miss => hardcore_sys::ShaderStage::RayMiss,
                glslang::ShaderStage::Callable => hardcore_sys::ShaderStage::RayCallable,
            };

            stage.into()
        }
    }

    impl TryFrom<ShaderStage> for glslang::ShaderStage {
        type Error = Error;

        fn try_from(value: ShaderStage) -> Result<Self, Self::Error> {
            match value.0 {
                hardcore_sys::ShaderStage::Vertex => Ok(glslang::ShaderStage::Vertex),
                hardcore_sys::ShaderStage::Fragment => Ok(glslang::ShaderStage::Fragment),
                hardcore_sys::ShaderStage::Compute => Ok(glslang::ShaderStage::Compute),
                hardcore_sys::ShaderStage::Mesh => Ok(glslang::ShaderStage::Mesh),
                hardcore_sys::ShaderStage::TesselationControl => {
                    Ok(glslang::ShaderStage::TesselationControl)
                }
                hardcore_sys::ShaderStage::TesselationEvaluation => {
                    Ok(glslang::ShaderStage::TesselationEvaluation)
                }
                hardcore_sys::ShaderStage::Geometry => Ok(glslang::ShaderStage::Geometry),
                hardcore_sys::ShaderStage::Task => Ok(glslang::ShaderStage::Task),
                hardcore_sys::ShaderStage::RayGeneration => Ok(glslang::ShaderStage::RayGeneration),
                hardcore_sys::ShaderStage::RayIntersection => Ok(glslang::ShaderStage::Intersect),
                hardcore_sys::ShaderStage::RayAnyHit => Ok(glslang::ShaderStage::AnyHit),
                hardcore_sys::ShaderStage::RayClosestHit => Ok(glslang::ShaderStage::ClosestHit),
                hardcore_sys::ShaderStage::RayMiss => Ok(glslang::ShaderStage::Miss),
                hardcore_sys::ShaderStage::RayCallable => Ok(glslang::ShaderStage::Callable),
                _ => Err(Error::UnknownStage((value.0 as i32).to_string())),
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
        ) -> Result<Vec<u32>, Error> {
            trace!("Compiling \"{stage:?}\" shader source to SPIR-V..");

            use glslang::{
                Compiler, CompilerOptions, ShaderInput, ShaderSource, Target, VulkanVersion,
            };

            let compiler = Compiler::acquire().ok_or(Error::NoCompiler)?;
            let vulkan = match vulkan_api_version() {
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

            let input = ShaderInput::new(&source, stage.try_into()?, &options, None, None)?;
            let shader = glslang::Shader::new(compiler, input)?;

            Ok(shader.compile()?)
        }

        pub fn try_from_source(
            source: &str,
            stage: ShaderStage,
            spirv_version: SpirvVersion,
        ) -> Result<Shader, Error> {
            Shader::try_from_bytecode(
                Shader::compile(source, stage, spirv_version)?.as_slice(),
                stage,
            )
        }

        pub async fn try_from_source_file<P: AsRef<Path>>(
            path: P,
            stage_hint: Option<ShaderStage>,
            spirv_version: SpirvVersion,
        ) -> Result<Shader, Error> {
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
pub use compilation::*;
