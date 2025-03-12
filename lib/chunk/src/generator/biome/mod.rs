mod deep_ocean;
mod desert;
mod grassland;
mod ocean;
mod vulcanic;

use noise::NoiseFn;

use super::tile;

pub struct Biome {
    pub deep_ocean: Box<dyn NoiseFn<f64, 2>>,
    pub ocean: Box<dyn NoiseFn<f64, 2>>,
    pub grassland: Box<dyn NoiseFn<f64, 2>>,
    pub desert: Box<dyn NoiseFn<f64, 2>>,
    pub vulcanic: Box<dyn NoiseFn<f64, 2>>,
}

impl Biome {
    pub fn new(seed: u32) -> Biome {
        Biome {
            deep_ocean: Box::new(deep_ocean::source(seed)),
            ocean: Box::new(ocean::source(seed)),
            grassland: Box::new(grassland::source(seed)),
            desert: Box::new(desert::source(seed)),
            vulcanic: Box::new(vulcanic::source(seed)),
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Biome {
        tile::Biome {
            deep_ocean: self.deep_ocean.get([x, z]),
            ocean: self.ocean.get([x, z]),
            grassland: self.grassland.get([x, z]),
            desert: self.desert.get([x, z]),
            vulcanic: self.vulcanic.get([x, z]),
        }
    }
}
