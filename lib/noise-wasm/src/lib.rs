use noise::{
    Billow, Fbm, HybridMulti, MultiFractal, NoiseFn, OpenSimplex, Perlin, RidgedMulti, Seedable,
    Simplex, Terrace, Value, Worley,
};
use wasm_bindgen::prelude::*;

pub fn seed_of(seed: String) -> u32 {
    let mut hash: u32 = 5381;

    for byte in seed.bytes() {
        hash = ((hash << 5) - hash) + byte as u32;
        hash |= 0;
    }

    hash
}

// ===== Basic noise generators =====

#[wasm_bindgen]
pub struct PerlinNoise {
    noise: Perlin,
}

#[wasm_bindgen]
impl PerlinNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Perlin::new(seed_of(seed));
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct SimplexNoise {
    noise: Simplex,
}

#[wasm_bindgen]
impl SimplexNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Simplex::new(seed_of(seed));
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct OpenSimplexNoise {
    noise: OpenSimplex,
}

#[wasm_bindgen]
impl OpenSimplexNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = OpenSimplex::new(seed_of(seed));
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct ValueNoise {
    noise: Value,
}

#[wasm_bindgen]
impl ValueNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Value::new(seed_of(seed));
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct WorleyNoise {
    noise: Worley,
}

#[wasm_bindgen]
impl WorleyNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Worley::new(seed_of(seed));
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

// ===== Fractal noise generators =====

#[wasm_bindgen]
pub struct FbmPerlin {
    noise: Fbm<Perlin>,
}

#[wasm_bindgen]
impl FbmPerlin {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Fbm::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct FbmSimplex {
    noise: Fbm<Simplex>,
}

#[wasm_bindgen]
impl FbmSimplex {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Fbm::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct FbmOpenSimplex {
    noise: Fbm<OpenSimplex>,
}

#[wasm_bindgen]
impl FbmOpenSimplex {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Fbm::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct RidgedPerlin {
    noise: RidgedMulti<Perlin>,
}

#[wasm_bindgen]
impl RidgedPerlin {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = RidgedMulti::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct BillowPerlin {
    noise: Billow<Perlin>,
}

#[wasm_bindgen]
impl BillowPerlin {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = Billow::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct HybridPerlin {
    noise: HybridMulti<Perlin>,
}

#[wasm_bindgen]
impl HybridPerlin {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        let noise = HybridMulti::new(seed_of(seed));
        Self { noise }
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise.octaves = octaves;
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise.frequency = frequency;
    }

    pub fn set_lacunarity(&mut self, lacunarity: f64) {
        self.noise.lacunarity = lacunarity;
    }

    pub fn set_persistence(&mut self, persistence: f64) {
        self.noise.persistence = persistence;
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}

#[wasm_bindgen]
pub struct BillowPerlinNoise {
    noise: Billow<Perlin>,
}

#[wasm_bindgen]
impl BillowPerlinNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> Self {
        Self {
            noise: Billow::new(seed_of(seed)),
        }
    }

    pub fn set_frequency(&mut self, frequency: f64) {
        self.noise = self.noise.clone().set_frequency(frequency);
    }

    pub fn set_octaves(&mut self, octaves: usize) {
        self.noise = self.noise.clone().set_octaves(octaves);
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }
}
