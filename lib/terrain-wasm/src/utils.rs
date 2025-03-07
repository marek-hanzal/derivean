use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct Noise {
    pub biome: f64,
    pub heightmap: f64,
    pub temperature: f64,
    pub moisture: f64,
    pub shade: f64,
}

pub fn seed_of(seed: &String) -> u32 {
    let mut hash: u32 = 5381;

    for byte in seed.bytes() {
        hash = ((hash << 5) - hash) + byte as u32;
        hash |= 0;
    }

    hash
}
