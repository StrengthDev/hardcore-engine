use crate::sync::RwLock;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use thiserror::Error;

static TOKEN: RwLock<Option<Arc<AtomicBool>>> = RwLock::new(None);

#[derive(Error, Debug)]
pub enum ContextTokenError {
    #[error("A context already exists")]
    AlreadyInitialised,
    #[error("No context currently exists")]
    NotInitialised,
}

pub(crate) struct ContextToken {
    value: Arc<AtomicBool>,
}

impl ContextToken {
    pub(crate) fn init_context() -> Result<(), ContextTokenError> {
        let mut guard = TOKEN.write();

        if guard.is_some() {
            return Err(ContextTokenError::AlreadyInitialised);
        }

        let _ = guard.insert(Arc::new(AtomicBool::new(true)));

        Ok(())
    }

    pub(crate) fn terminate_context() -> Result<(), ContextTokenError> {
        let mut guard = TOKEN.write();

        if guard.is_none() {
            return Err(ContextTokenError::NotInitialised);
        }

        if let Some(value) = guard.take() {
            value.store(false, Ordering::Release);
        } else {
            unreachable!("Token must have been set to some value")
        }

        Ok(())
    }

    pub(crate) fn new() -> Result<ContextToken, ContextTokenError> {
        let guard = TOKEN.read();

        if let Some(value) = guard.as_ref() {
            return Ok(ContextToken {
                value: value.clone(),
            });
        }

        Err(ContextTokenError::NotInitialised)
    }

    pub(crate) fn valid(&self) -> bool {
        self.value.load(Ordering::Acquire)
    }

    pub(crate) fn ok(&self) -> Result<(), ContextTokenError> {
        if !self.valid() {
            Err(ContextTokenError::NotInitialised)
        } else {
            Ok(())
        }
    }
}

/// Instances of objects implementing this trait are only valid for the lifetime of the context
/// which was active at the time of their creation.
pub(crate) trait ContextDependent {
    /// Check if this instance is valid within the current context.
    fn valid(&self) -> bool;
}
