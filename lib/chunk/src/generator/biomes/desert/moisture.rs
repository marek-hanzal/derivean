use noise::{NoiseFn, Perlin};

pub fn moisture(seed: u32) -> impl NoiseFn<f64, 2> {
    Perlin::new(seed)
}
