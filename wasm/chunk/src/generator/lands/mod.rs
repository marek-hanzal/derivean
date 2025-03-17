mod hill;
mod mountain;
mod plain;
mod platau;
mod valley;

use noise::NoiseFn;

use super::tile;

pub struct Lands {
    pub hill: super::Land,
    pub mountain: super::Land,
    pub plain: super::Land,
    pub platau: super::Land,
    pub valley: super::Land,
}

impl Lands {
    pub fn new(seed: u32) -> Lands {
        Lands {
            hill: super::Land {
                weight: Box::new(hill::weight(seed)),
                height: Box::new(hill::height(seed)),
            },
            mountain: super::Land {
                weight: Box::new(mountain::weight(seed)),
                height: Box::new(mountain::height(seed)),
            },
            plain: super::Land {
                weight: Box::new(plain::weight(seed)),
                height: Box::new(plain::height(seed)),
            },
            platau: super::Land {
                weight: Box::new(platau::weight(seed)),
                height: Box::new(platau::height(seed)),
            },
            valley: super::Land {
                weight: Box::new(valley::weight(seed)),
                height: Box::new(valley::height(seed)),
            },
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Lands {
        tile::Lands {
            hill: tile::Land {
                weight: self.hill.weight.get([x, z]),
                height: self.hill.height.get([x, z]),
            },
            mountain: tile::Land {
                weight: self.mountain.weight.get([x, z]),
                height: self.mountain.height.get([x, z]),
            },
            plain: tile::Land {
                weight: self.plain.weight.get([x, z]),
                height: self.plain.height.get([x, z]),
            },
            platau: tile::Land {
                weight: self.platau.weight.get([x, z]),
                height: self.platau.height.get([x, z]),
            },
            valley: tile::Land {
                weight: self.valley.weight.get([x, z]),
                height: self.valley.height.get([x, z]),
            },
        }
    }
}
