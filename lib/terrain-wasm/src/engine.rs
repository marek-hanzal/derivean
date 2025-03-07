use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct Engine {
    //
}

impl Engine {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Engine {
        Engine {}
    }
}
