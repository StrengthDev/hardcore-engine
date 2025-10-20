use std::marker::PhantomData;

/// Small wrapper of a native handle with a lifetime bound.
///
/// Typically used to bind resource handles to the instance's [`State`][crate::state::State].
pub(super) struct DependentHandle<'s, T> {
    pub(super) inner: T,
    phantom: PhantomData<&'s T>,
}

impl<'s, T> DependentHandle<'s, T> {
    pub(super) fn ptr(&self) -> *const T {
        &raw const self.inner
    }

    pub(super) fn mut_ptr(&mut self) -> *mut T {
        &raw mut self.inner
    }
}

impl<'s, T> From<T> for DependentHandle<'s, T> {
    fn from(value: T) -> Self {
        Self {
            inner: value,
            phantom: PhantomData,
        }
    }
}
