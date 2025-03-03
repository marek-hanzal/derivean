use noise::{
    Billow, Fbm, HybridMulti, MultiFractal, NoiseFn, OpenSimplex, Perlin, RidgedMulti, Simplex,
    Value, Worley,
};
use wasm_bindgen::prelude::*;

// ===== Basic noise generators =====

#[wasm_bindgen]
pub fn bello() -> u32 {
    return 42;
}

#[wasm_bindgen]
pub struct PerlinNoise {
    noise: Perlin,
}

#[wasm_bindgen]
impl PerlinNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: u32) -> Self {
        let noise = Perlin::new(seed);
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }

    pub fn get_3d(&self, x: f64, y: f64, z: f64) -> f64 {
        self.noise.get([x, y, z])
    }
}

#[wasm_bindgen]
pub struct SimplexNoise {
    noise: Simplex,
}

#[wasm_bindgen]
impl SimplexNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: u32) -> Self {
        let noise = Simplex::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = OpenSimplex::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = Value::new(seed);
        Self { noise }
    }

    pub fn get(&self, x: f64, y: f64) -> f64 {
        self.noise.get([x, y])
    }

    pub fn get_3d(&self, x: f64, y: f64, z: f64) -> f64 {
        self.noise.get([x, y, z])
    }
}

#[wasm_bindgen]
pub struct WorleyNoise {
    noise: Worley,
}

#[wasm_bindgen]
impl WorleyNoise {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: u32) -> Self {
        let noise = Worley::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = Fbm::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = Fbm::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = Fbm::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = RidgedMulti::new(seed);
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
    pub fn new(seed: u32) -> Self {
        let noise = Billow::new(seed);
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

    pub fn get_3d(&self, x: f64, y: f64, z: f64) -> f64 {
        self.noise.get([x, y, z])
    }
}

#[wasm_bindgen]
pub struct HybridPerlin {
    noise: HybridMulti<Perlin>,
}

#[wasm_bindgen]
impl HybridPerlin {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: u32) -> Self {
        let noise = HybridMulti::new(seed);
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
