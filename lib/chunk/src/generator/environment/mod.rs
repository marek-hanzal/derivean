mod moisture;
mod temp;

use noise::NoiseFn;

use super::tile;

pub struct Environment {
    pub temp: Box<dyn NoiseFn<f64, 2>>,
    pub moisture: Box<dyn NoiseFn<f64, 2>>,
}

impl Environment {
    pub fn new(seed: u32) -> Environment {
        Environment {
            temp: Box::new(temp::source(seed)),
            moisture: Box::new(moisture::source(seed)),
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Environment {
        tile::Environment {
            temp: self.temp.get([x, z]),
            moisture: self.moisture.get([x, z]),
        }
    }
}
