//! A layer is an abstract concept used to represent some component of the application. Some layer
//! examples could be the global application state, a *UI* component, the main scene, etc.
//!
//! # The layer stack
//!
//! Within **Hardcore**'s global context is the layer stack. This is the core of the application, as
//! all layers within the stack are what makes up the application. Layers get pushed and popped from
//! the stack according to the application, through calls to [`push_layer`][crate::push_layer],
//! [`pop_layer`][crate::pop_layer] and [`pop_layers`][crate::pop_layers]. Each frame, from the
//! bottom to the top of the stack, each layer is updated (its [`Layer::tick`] function is called).
//!
//! The scope of a layer within the stack should be  narrower in higher positions, that is, layers
//! at the bottom should represent things like the global application state while layers at the
//! top should represent more specific things like *UI* elements. This isn't an enforced rule, but
//! it will probably make layer management easier and ties well with the way events are handle by
//! layers in the stack (layers at the top get to try handling events first).

use crate::event::Event;

/// An abstract application layer, used to represent some component of the application.
#[async_trait::async_trait]
pub trait Layer: Send {
    /// Update this layer for the current frame.
    ///
    /// This function is called once per frame, with the timing depending on this layer's position
    /// in the stack, bottom layers get updated first while top layers get updated last.
    async fn tick(&mut self, context: &Context);

    /// Try to handle a system event.
    ///
    /// # Return
    ///
    /// **`true`** if the layer has handled the event, **`false`** otherwise. If the event was
    /// handled, then it won't get propagated further down the layer stack.
    async fn handle_event(&mut self, event: &Event) -> bool;
}

/// Global context information to be passed to a layer during execution.
pub struct Context {
    /// The current number of layers in the global context.
    pub layer_count: usize,

    /// The current layer's index in the global context's layer stack.
    pub current_layer_idx: usize,

    /// The current frame.
    ///
    /// Frame count begins when the [main loop][crate::run] starts, and is reset when it ends.
    pub frame: usize,

    /// How long it took to process the previous frame, in seconds.
    pub delta_time: f64,

    /// A runtime which may be used by layers to spawn tasks.
    pub worker_pool: tokio::runtime::Handle,
}

impl Context {
    pub(crate) fn new(worker_pool: tokio::runtime::Handle) -> Self {
        Self {
            layer_count: 0,
            current_layer_idx: 0,
            frame: 0,
            delta_time: 0.0,
            worker_pool,
        }
    }
}
