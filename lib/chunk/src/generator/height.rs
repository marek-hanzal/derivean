use noise::{NoiseFn, Perlin};

pub fn height(seed: u32) -> impl NoiseFn<f64, 2> {
    Perlin::new(seed)
}
