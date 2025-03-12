mod hill;
mod mountain;
mod plain;
mod platau;
mod valley;

use noise::NoiseFn;

use super::tile;

pub struct Land {
    pub hill: Box<dyn NoiseFn<f64, 2>>,
    pub mountain: Box<dyn NoiseFn<f64, 2>>,
    pub plain: Box<dyn NoiseFn<f64, 2>>,
    pub platau: Box<dyn NoiseFn<f64, 2>>,
    pub valley: Box<dyn NoiseFn<f64, 2>>,
}

impl Land {
    pub fn new(seed: u32) -> Land {
        Land {
            hill: Box::new(hill::source(seed)),
            mountain: Box::new(mountain::source(seed)),
            plain: Box::new(plain::source(seed)),
            platau: Box::new(platau::source(seed)),
            valley: Box::new(valley::source(seed)),
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Land {
        tile::Land {
            hill: self.hill.get([x, z]),
            mountain: self.mountain.get([x, z]),
            plain: self.plain.get([x, z]),
            platau: self.platau.get([x, z]),
            valley: self.valley.get([x, z]),
        }
    }
}
