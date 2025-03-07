use super::noise::Noise;

use wasm_bindgen::prelude::wasm_bindgen;

#[wasm_bindgen]
pub struct Engine {
    pub noise: Noise,
}

#[wasm_bindgen]
impl Engine {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        Self {
            noise: Noise::new(seed),
        }
    }

    /**
     * Just a simple test function.
     */
    pub fn foo(&self) -> String {
        String::from("foo")
    }
}
