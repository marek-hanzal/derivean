use noise::{NoiseFn, Perlin};

pub fn weight(seed: u32) -> impl NoiseFn<f64, 2> {
    Perlin::new(seed)
}
