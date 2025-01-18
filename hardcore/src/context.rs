use crate::device::Device;
use crate::layer::Layer;
use std::rc::Rc;

/// Global context information to be passed to a layer during execution.
pub struct Context<'l> {
    pub(super) running: bool,

    pub(super) pushed_layers: Vec<Box<dyn Layer<'l> + 'l>>,

    pub(super) layer_pop_count: usize,

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

    /// The list of devices available for use.
    pub devices: Rc<Vec<Device>>,
}

impl<'l> Context<'l> {
    pub(super) fn create(device_count: u32) -> Self {
        let devices: Vec<_> = (0..device_count).map(Device::new).collect();
        Self {
            running: false,
            pushed_layers: vec![],
            layer_pop_count: 0,
            layer_count: 0,
            current_layer_idx: 0,
            frame: 0,
            delta_time: 0.0,
            devices: Rc::new(devices),
        }
    }

    /// If the engine is running, stop it and exit out of the main loop ([`run`]).
    pub fn exit(&mut self) {
        self.running = false;
    }

    /// Submit a new layer to be pushed into the layer stack in the next frame.
    ///
    /// # Parameters
    /// * `layer` - The layer to be pushed.
    pub fn push_layer(&mut self, layer: impl Layer<'l> + 'l) {
        self.pushed_layers.push(Box::new(layer));
    }

    // TODO change pop layer functionality to keep users from popping beyond the current layer

    /// Increment the number of layers to be popped in the next frame by 1.
    pub fn pop_layer(&mut self) {
        self.layer_pop_count += 1;
    }

    /// Increase the number of layers to be popped in the next frame by an arbitrary amount.
    ///
    /// # Parameters
    /// * `count` - The additional number of layers to be popped.
    pub fn pop_layers(&mut self, count: usize) {
        self.layer_pop_count += count;
    }
}
