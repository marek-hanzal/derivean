mod derivean;

pub use derivean::Engine;
pub use derivean::Tile;
use wasm_bindgen::prelude::*;

#[wasm_bindgen(start)]
pub fn init() {
    console_error_panic_hook::set_once();
}
