mod generator;

use generator::Source;
use wasm_bindgen::prelude::wasm_bindgen;

pub fn seed_of(seed: &String) -> u32 {
    let mut hash: u32 = 5381;

    for byte in seed.bytes() {
        hash = ((hash << 5) - hash) + byte as u32;
        hash |= 0;
    }

    hash
}

/**
 * Base noise generator used to generate a tile.
 */
#[wasm_bindgen]
pub struct Chunk {
    source: Source,
}

#[wasm_bindgen]
impl Chunk {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Chunk {
        Chunk {
            source: Source::new(seed_of(&seed)),
        }
    }

    #[wasm_bindgen]
    pub fn tile(&self, x: f64, z: f64) -> generator::Tile {
        self.source.generate(x, z)
    }
}
