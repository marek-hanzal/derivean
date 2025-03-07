use wasm_bindgen::prelude::wasm_bindgen;

use super::seed_of;

// https://github.com/Razaekel/noise-rs/blob/develop/examples/complexplanet.rs

/**
 * Value of a generate tile with all it's information needed to
 * render it or evaluate properties used for a gameplay.
 *
 * All values are -1 to 1.
 */
#[wasm_bindgen]
pub struct Tile {
    /**
     * Height of a tile.
     */
    pub height: f64,
    /**
     * Biome defines a base biome type of a tile.
     *
     * This works in conjuction with other values to compute the final biome.
     */
    pub biome: f64,
    /**
     * Temperature of a tile. Used to modify biome.
     */
    pub temp: f64,
    /**
     * Moisture on this tile. Used to modify biome.
     */
    pub moisture: f64,
    /**
     * Pure cosmetic value used to adjust the shade (color) of a tile.
     */
    pub shade: f64,
}

#[wasm_bindgen]
impl Tile {
    #[wasm_bindgen(constructor)]
    pub fn new(height: f64, biome: f64, temp: f64, moisture: f64, shade: f64) -> Tile {
        Tile {
            height,
            biome,
            temp,
            moisture,
            shade,
        }
    }
}

/**
 * Base noise generator used to generate a tile.
 */
#[wasm_bindgen(getter_with_clone)]
#[derive(Clone, Copy)]
pub struct Noise {
    pub seed: u32,
}

#[wasm_bindgen]
impl Noise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Noise {
        Noise {
            seed: seed_of(&seed),
        }
    }

    pub fn value(&self) -> Tile {
        Tile::new(1., 1., 1., 1., 1.)
    }
}
