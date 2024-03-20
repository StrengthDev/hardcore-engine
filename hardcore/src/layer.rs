use crate::event::Event;
use tokio::sync::mpsc::UnboundedSender;

#[async_trait::async_trait]
pub trait Layer: Send {
    async fn tick(&mut self, context: &Context);

    async fn handle_event(&mut self, event: &Event) -> bool {
        false
    }
}

pub struct Context {
    pub layer_count: usize,
    pub current_layer_idx: usize,
}

impl Context {
    pub(crate) fn new() -> Self {
        Self {
            layer_count: 0,
            current_layer_idx: 0,
        }
    }
}
