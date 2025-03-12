use noise::{
    Billow, Blend, Fbm, MultiFractal, NoiseFn, Perlin, RidgedMulti, ScaleBias, Seedable, Turbulence,
};

pub fn source(seed: u32) -> impl NoiseFn<f64, 2> {
    // Parameters for deep ocean
    const BASE_FREQUENCY: f64 = 0.125;
    const TURBULENCE_FREQUENCY: f64 = 1.0;
    const TURBULENCE_POWER: f64 = 0.25;

    // Base noise for deep ocean depth variation
    // Use ridged multi-fractal for deeper trenches and canyons
    let ridged = RidgedMulti::<Perlin>::new(seed)
        .set_frequency(BASE_FREQUENCY * 0.5)
        .set_lacunarity(2.18)
        .set_persistence(0.65)
        .set_octaves(6);

    // Scale the ridged noise to create deeper trenches
    let base_ridged = ScaleBias::new(ridged).set_scale(0.65).set_bias(-0.8);

    // Billow noise for smoother ocean floor areas
    let billow = Billow::<Perlin>::new(seed + 1)
        .set_frequency(BASE_FREQUENCY * 0.3)
        .set_lacunarity(2.0)
        .set_persistence(0.5)
        .set_octaves(4);

    // Scale billow to be slightly higher than ridged
    let base_billow = ScaleBias::new(billow).set_scale(0.45).set_bias(-0.6);

    // Perlin noise for the blend control
    let control = Fbm::<Perlin>::new(seed + 2)
        .set_frequency(BASE_FREQUENCY * 0.2)
        .set_lacunarity(2.0)
        .set_persistence(0.5)
        .set_octaves(4);

    // Blend between ridged and billow to create varied ocean floor
    let blend = Blend::new(base_billow, base_ridged, control);

    // Add turbulence for more realistic and chaotic ocean floor
    let turbulence = Turbulence::<_, Perlin>::new(blend)
        .set_seed(seed + 3)
        .set_frequency(TURBULENCE_FREQUENCY)
        .set_power(TURBULENCE_POWER)
        .set_roughness(3);

    turbulence
}
