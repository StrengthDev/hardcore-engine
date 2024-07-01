pub enum Kind {
    Uniform,
    Storage,
}

pub trait GenericBuffer {
    fn kind(self) -> Kind;
}

pub trait ArrayBuffer: GenericBuffer {
    fn len(self) -> usize;
}

pub trait VectorBuffer: GenericBuffer {
    fn len(self) -> usize;

    fn capacity(self) -> usize;
}

pub mod local {
    pub struct UniformArray {}

    pub struct UniformVector {}

    pub struct StorageArray {}

    pub struct StorageVector {}
}

pub mod dynamic {
    pub struct UniformArray {}

    pub struct UniformVector {}

    pub struct StorageArray {}

    pub struct StorageVector {}
}

pub mod host {
    pub struct UniformArray {}

    pub struct UniformVector {}

    pub struct StorageArray {}

    pub struct StorageVector {}
}
