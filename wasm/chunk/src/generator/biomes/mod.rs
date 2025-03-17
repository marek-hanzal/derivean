mod deep_ocean;
mod desert;
mod grassland;
mod ocean;
mod vulcanic;

use noise::NoiseFn;

use super::tile;

pub struct Biomes {
    pub deep_ocean: super::Biome,
    pub ocean: super::Biome,
    pub grassland: super::Biome,
    pub desert: super::Biome,
    pub vulcanic: super::Biome,
}

impl Biomes {
    pub fn new(seed: u32) -> Biomes {
        Biomes {
            deep_ocean: super::Biome {
                weight: Box::new(deep_ocean::weight(seed)),
                temp: Box::new(deep_ocean::temp(seed)),
                moisture: Box::new(deep_ocean::moisture(seed)),
            },
            ocean: super::Biome {
                weight: Box::new(ocean::weight(seed)),
                temp: Box::new(ocean::temp(seed)),
                moisture: Box::new(ocean::moisture(seed)),
            },
            grassland: super::Biome {
                weight: Box::new(grassland::weight(seed)),
                temp: Box::new(grassland::temp(seed)),
                moisture: Box::new(grassland::moisture(seed)),
            },
            desert: super::Biome {
                weight: Box::new(desert::weight(seed)),
                temp: Box::new(desert::temp(seed)),
                moisture: Box::new(desert::moisture(seed)),
            },
            vulcanic: super::Biome {
                weight: Box::new(vulcanic::weight(seed)),
                temp: Box::new(vulcanic::temp(seed)),
                moisture: Box::new(vulcanic::moisture(seed)),
            },
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Biomes {
        tile::Biomes {
            deep_ocean: tile::Biome {
                weight: self.deep_ocean.weight.get([x, z]),
                temp: self.deep_ocean.temp.get([x, z]),
                moisture: self.deep_ocean.moisture.get([x, z]),
            },
            ocean: tile::Biome {
                weight: self.ocean.weight.get([x, z]),
                temp: self.ocean.temp.get([x, z]),
                moisture: self.ocean.moisture.get([x, z]),
            },
            grassland: tile::Biome {
                weight: self.grassland.weight.get([x, z]),
                temp: self.grassland.temp.get([x, z]),
                moisture: self.grassland.moisture.get([x, z]),
            },
            desert: tile::Biome {
                weight: self.desert.weight.get([x, z]),
                temp: self.desert.temp.get([x, z]),
                moisture: self.desert.moisture.get([x, z]),
            },
            vulcanic: tile::Biome {
                weight: self.vulcanic.weight.get([x, z]),
                temp: self.vulcanic.temp.get([x, z]),
                moisture: self.vulcanic.moisture.get([x, z]),
            },
        }
    }
}
