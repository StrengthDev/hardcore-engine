//! Definitions for synchronisation types, based on enabled features.

use std::ops::Deref;

#[cfg(feature = "parking_lot")]
pub(crate) type Mutex<T> = parking_lot::Mutex<T>;
#[cfg(not(feature = "parking_lot"))]
pub(crate) type Mutex<T> = std::sync::Mutex<T>;

#[cfg(feature = "parking_lot")]
pub(crate) type RwLock<T> = parking_lot::RwLock<T>;
#[cfg(not(feature = "parking_lot"))]
pub(crate) type RwLock<T> = std::sync::RwLock<T>;

#[cfg(feature = "parking_lot")]
pub(crate) type Condvar = parking_lot::Condvar;
#[cfg(not(feature = "parking_lot"))]
pub(crate) type Condvar = std::sync::Condvar;
