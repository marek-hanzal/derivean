mod environment;
mod land;
mod tile;

use environment::Environment;
use land::Land;
pub use tile::Tile;

/**
 * This is a source for all available noises for the generator, separated into categories.
 */
pub struct Source {
    /**
     * Noises used to generate land-related noises. This should be used to compute the kind of
     * terrain on a given tile, e.g. mountain, valley, plain, etc.
     */
    pub land: Land,
    /**
     * Noises used to generate environment-related noises. This should be used to compute the
     * environment of a given tile, e.g. temperature, moisture, etc.
     */
    pub environment: Environment,
}

impl Source {
    pub fn new(seed: u32) -> Source {
        Source {
            land: Land::new(seed),
            environment: Environment::new(seed),
        }
    }

    pub fn generate(&self, x: f64, z: f64) -> tile::Tile {
        let tile = tile::Tile {
            land: self.land.generate(x, z),
            environment: self.environment.generate(x, z),
        };

        tile
    }
}
