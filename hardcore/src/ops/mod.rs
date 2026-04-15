//! Device operations module.

pub mod raster_pipeline;
pub mod render_pass;

use std::num::NonZeroU8;

mod seal {
    pub trait Seal {}
}
pub trait Dependency: seal::Seal {
    fn as_dependency(&self) -> hardcore_sys::Dependency;
}

fn as_dependency<D: Dependency + ?Sized>(dependency: Option<&D>) -> hardcore_sys::Dependency {
    dependency.map_or(
        hardcore_sys::Dependency::default(),
        Dependency::as_dependency,
    )
}

impl_stateful_fn!(Predicate(frame: usize) -> bool, predicate_impl);

impl<UserPredicate: Fn(usize) -> bool + 'static> From<Schedule<UserPredicate>>
    for Option<Predicate>
{
    fn from(value: Schedule<UserPredicate>) -> Self {
        match value {
            Schedule::EveryFrame => None,
            Schedule::Stride(frame_stride) => {
                if frame_stride.get() == 1 {
                    None
                } else {
                    Some(Predicate::new(move |frame| {
                        (frame % frame_stride.get() as usize) == 0
                    }))
                }
            }
            Schedule::Predicate(predicate_fn) => Some(Predicate::new(predicate_fn)),
        }
    }
}

/// An operation's schedule. This defines if an operation should be executed within a frame.
#[derive(Default)]
pub enum Schedule<UserPredicate: Fn(usize) -> bool + 'static> {
    /// The operation is executed every frame.
    #[default]
    EveryFrame,

    /// The operation will be executed every `x` frames.
    /// (If `x` is 1, this is equivalent to [`Schedule::EveryFrame`])
    Stride(NonZeroU8),

    /// The operation will get executed within a frame if the predicate function returns [`true`].
    Predicate(UserPredicate),
}
