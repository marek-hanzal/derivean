use wasm_bindgen::prelude::wasm_bindgen;

#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Land {
    pub mountain: f64,
    pub platau: f64,
    pub valley: f64,
    pub hill: f64,
    pub plain: f64,
}

#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Environment {
    pub temp: f64,
    pub moisture: f64,
}

/**
 * Value of a generate tile with all it's information needed to
 * render it or evaluate properties used for a gameplay.
 *
 * All values are -1 to 1.
 */
#[wasm_bindgen]
pub struct Tile {
    pub land: Land,
    pub environment: Environment,
}
