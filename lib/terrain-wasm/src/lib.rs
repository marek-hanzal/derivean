use noise::MultiFractal;
use noise::{Fbm, NoiseFn, Perlin};
use wasm_bindgen::prelude::*;

pub fn seed_of(seed: &String) -> u32 {
    let mut hash: u32 = 5381;

    for byte in seed.bytes() {
        hash = ((hash << 5) - hash) + byte as u32;
        hash |= 0;
    }

    hash
}

#[wasm_bindgen(start)]
pub fn init() {
    console_error_panic_hook::set_once();
}

#[wasm_bindgen]
pub struct Noise {
    pub biome: f64,
    pub heightmap: f64,
    pub temperature: f64,
    pub moisture: f64,
    pub shade: f64,
}

#[wasm_bindgen]
pub struct TerrainGenerator {
    base: Fbm<Perlin>,
    perlin: Fbm<Perlin>,
}

#[wasm_bindgen]
impl TerrainGenerator {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> TerrainGenerator {
        let seed = seed_of(&seed);

        const CONTINEN_FREQUENCY: f64 = 1.;
        const CONTINEN_LACUNARITY: f64 = 2.208984375;

        TerrainGenerator {
            base: Fbm::<Perlin>::new(seed)
                .set_frequency(CONTINEN_FREQUENCY)
                .set_persistence(0.5)
                .set_lacunarity(CONTINEN_LACUNARITY)
                .set_octaves(14),
            perlin: Fbm::<Perlin>::new(seed + 1)
                .set_frequency(CONTINEN_FREQUENCY * 4.34375)
                .set_persistence(0.5)
                .set_lacunarity(CONTINEN_LACUNARITY)
                .set_octaves(11),
        }
    }

    pub fn coord(&self, x: f64, z: f64) -> Noise {
        Noise {
            biome: self.base.get([x, z, 0.0]),
            heightmap: self.perlin.get([x, z, 0.0]),
            temperature: self.perlin.get([x, z, 0.0]),
            moisture: self.perlin.get([x, z, 0.0]),
            shade: self.perlin.get([x, z, 0.0]),
        }
    }
}
