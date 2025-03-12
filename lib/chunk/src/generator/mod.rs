mod biomes;
mod lands;
mod tile;

use noise::NoiseFn;
pub use tile::Tile;

pub struct Land {
    pub weight: Box<dyn NoiseFn<f64, 2>>,
    pub height: Box<dyn NoiseFn<f64, 2>>,
}

pub struct Biome {
    pub weight: Box<dyn NoiseFn<f64, 2>>,
    pub temp: Box<dyn NoiseFn<f64, 2>>,
    pub moisture: Box<dyn NoiseFn<f64, 2>>,
}

/**
 * This is a source for all available noises for the generator, separated into categories.
 */
pub struct Source {
    /**
     * Noises used to generate land-related noises. This should be used to compute the kind of
     * terrain on a given tile, e.g. mountain, valley, plain, etc.
     */
    pub lands: lands::Lands,
    /**
     * Individual biome noises used to generate biome-related noises. This should be used to compute
     * the biome of a given tile, e.g. grassland, desert, etc.
     */
    pub biomes: biomes::Biomes,
}

impl Source {
    pub fn new(seed: u32) -> Source {
        Source {
            lands: lands::Lands::new(seed),
            biomes: biomes::Biomes::new(seed),
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Tile {
        tile::Tile {
            lands: self.lands.generate(x, z),
            biomes: self.biomes.generate(x, z),
        }
    }
}
