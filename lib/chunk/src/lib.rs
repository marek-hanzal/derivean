mod generator;
mod utils;

use generator::{biome, height, moisture, shade, temp};
use noise::NoiseFn;
use utils::seed_of;
use wasm_bindgen::prelude::wasm_bindgen;

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
#[wasm_bindgen]
pub struct Chunk {
    height: Box<dyn NoiseFn<f64, 2>>,
    biome: Box<dyn NoiseFn<f64, 2>>,
    temp: Box<dyn NoiseFn<f64, 2>>,
    moisture: Box<dyn NoiseFn<f64, 2>>,
    shade: Box<dyn NoiseFn<f64, 2>>,
}

#[wasm_bindgen]
impl Chunk {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Chunk {
        let seed = seed_of(&seed);
        Chunk {
            height: Box::new(height(seed)),
            biome: Box::new(biome(seed)),
            temp: Box::new(temp(seed)),
            moisture: Box::new(moisture(seed)),
            shade: Box::new(shade(seed)),
        }
    }

    pub fn tile(&self, x: f64, z: f64) -> Tile {
        Tile::new(
            self.height.get([x, z]),
            self.biome.get([x, z]),
            self.temp.get([x, z]),
            self.moisture.get([x, z]),
            self.shade.get([x, z]),
        )
    }
}
