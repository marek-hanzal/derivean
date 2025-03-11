use noise::{
    Abs, Billow, Blend, Cache, Clamp, Displace, Fbm, MultiFractal, NoiseFn, Perlin, RidgedMulti,
    ScaleBias, Select, Terrace, Turbulence, Worley,
};

pub fn height(seed: u32) -> impl NoiseFn<f64, 2> {
    let base_layer = Billow::<Worley>::new(seed)
        .set_frequency(0.025)
        .set_lacunarity(2.2557434)
        .set_octaves(8);

    let rigged = RidgedMulti::<Perlin>::new(seed)
        .set_frequency(0.025)
        .set_lacunarity(2.2557434)
        .set_octaves(8);

    let fbm = Fbm::<Perlin>::new(seed)
        .set_frequency(0.025)
        .set_lacunarity(2.2557434)
        .set_octaves(8);

    let blend: Blend<_, Billow<Worley>, RidgedMulti<Perlin>, Fbm<Perlin>, 2> =
        Blend::new(base_layer.clone(), rigged.clone(), fbm.clone());

    let clamp = Clamp::new(blend.clone()).set_bounds(0.0, 0.5);

    let select = Select::new(clamp.clone(), rigged.clone(), fbm.clone())
        .set_bounds(-0.25, 0.25)
        .set_falloff(0.5);

    // let turbulence = Turbulence::<_, Perlin>::new(select.clone());

    // let terrace: Terrace<_, Billow<Worley>, 2> = Terrace::new(base_layer.clone());

    // let tuble = Blend::new(terrace.clone(), turbulence.clone(), fbm.clone());

    Cache::new(Abs::new(select))
}
