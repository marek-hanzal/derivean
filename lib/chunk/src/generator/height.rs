use noise::{
    Cache, Clamp, Curve, Fbm, Min, MultiFractal, NoiseFn, Perlin, ScaleBias, Seedable, Select,
    Turbulence,
};

pub fn height(seed: u32) -> impl NoiseFn<f64, 2> {
    /// Frequency of the planet's continents. Higher frequency produces
    /// smaller, more numerous continents. This value is measured in radians.
    const CONTINENT_FREQUENCY: f64 = 0.125 / 8.;

    /// Lacunarity of the planet's continents. Changing this value produces
    /// slightly different continents. For the best results, this value should
    /// be random, but close to 2.0.
    const CONTINENT_LACUNARITY: f64 = 2.208984375;

    /// Specifies the planet's sea level. This value must be between -1.0
    /// (minimum planet elevation) and +1.0 (maximum planet elevation).
    const SEA_LEVEL: f64 = 0.0;

    fn base_continent_def(seed: u32) -> impl NoiseFn<f64, 2> {
        let base_continent_def_fb0 = Fbm::<Perlin>::new(seed)
            .set_frequency(CONTINENT_FREQUENCY)
            .set_persistence(0.5)
            .set_lacunarity(CONTINENT_LACUNARITY)
            .set_octaves(14);

        // 2: [Continent-with-ranges module]: Next, a curve module modifies the
        // output value from the continent module so that very high values appear
        // near sea level. This defines the positions of the mountain ranges.
        let base_continent_def_cu = Curve::new(base_continent_def_fb0)
            .add_control_point(-2.0000 + SEA_LEVEL, -1.625 + SEA_LEVEL)
            .add_control_point(-1.0000 + SEA_LEVEL, -1.375 + SEA_LEVEL)
            .add_control_point(0.0000 + SEA_LEVEL, -0.375 + SEA_LEVEL)
            .add_control_point(0.0625 + SEA_LEVEL, 0.125 + SEA_LEVEL)
            .add_control_point(0.1250 + SEA_LEVEL, 0.250 + SEA_LEVEL)
            .add_control_point(0.2500 + SEA_LEVEL, 1.000 + SEA_LEVEL)
            .add_control_point(0.5000 + SEA_LEVEL, 0.250 + SEA_LEVEL)
            .add_control_point(0.7500 + SEA_LEVEL, 0.250 + SEA_LEVEL)
            .add_control_point(1.0000 + SEA_LEVEL, 0.500 + SEA_LEVEL)
            .add_control_point(2.0000 + SEA_LEVEL, 0.500 + SEA_LEVEL);

        // 3: [Carver module]: This higher-frequency BasicMulti module will be
        // used by subsequent noise functions to carve out chunks from the
        // mountain ranges within the continent-with-ranges module so that the
        // mountain ranges will not be completely impassible.
        let base_continent_def_fb1 = Fbm::<Perlin>::new(seed + 1)
            .set_frequency(CONTINENT_FREQUENCY * 4.34375)
            .set_persistence(0.5)
            .set_lacunarity(CONTINENT_LACUNARITY)
            .set_octaves(11);

        // 4: [Scaled-carver module]: This scale/bias module scales the output
        // value from the carver module such that it is usually near 1.0. This
        // is required for step 5.
        let base_continent_def_sb = ScaleBias::new(base_continent_def_fb1)
            .set_scale(0.375)
            .set_bias(0.625);

        // 5: [Carved-continent module]: This minimum-value module carves out
        // chunks from the continent-with-ranges module. it does this by ensuring
        // that only the minimum of the output values from the scaled-carver
        // module and the continent-with-ranges module contributes to the output
        // value of this subgroup. Most of the time, the minimum value module will
        // select the output value from the continent-with-ranges module since the
        // output value from the scaled-carver is usually near 1.0. Occasionally,
        // the output from the scaled-carver module will be less than the output
        // value from the continent-with-ranges module, so in this case, the output
        // value from the scaled-carver module is selected.
        let base_continent_def_mi = Min::new(base_continent_def_sb, base_continent_def_cu);

        // 6: [Clamped-continent module]: Finally, a clamp module modifies the
        // carved continent module to ensure that the output value of this subgroup
        // is between -1.0 and 1.0.
        let base_continent_def_cl = Clamp::new(base_continent_def_mi).set_bounds(-1.0, 1.0);

        // 7: [Base-continent-definition subgroup]: Caches the output value from
        // the clamped-continent module.
        let base_continent_def = Cache::new(base_continent_def_cl);

        base_continent_def
    }

    fn continent_tu_def(
        seed: u32,
    ) -> Cache<
        Select<
            f64,
            impl NoiseFn<f64, 2>,
            Turbulence<Turbulence<Turbulence<impl NoiseFn<f64, 2>, Perlin>, Perlin>, Perlin>,
            impl NoiseFn<f64, 2>,
            2,
        >,
    > {
        // 1: [Coarse-turbulence module]: This turbulence module warps the output
        // value from the base-continent-definition subgroup, adding some coarse
        // detail to it.
        let continent_def_tu0 = Turbulence::<_, Perlin>::new(base_continent_def(seed))
            .set_seed(seed + 10)
            .set_frequency(CONTINENT_FREQUENCY * 15.25)
            .set_power(CONTINENT_FREQUENCY / 113.75)
            .set_roughness(13);

        // 2: [Intermediate-turbulence module]: This turbulence module warps the
        // output value from the coarse-turbulence module. This turbulence has a
        // higher frequency, but lower power, than the coarse-turbulence module,
        // adding some intermediate detail to it.
        let continent_def_tu1 = Turbulence::<_, Perlin>::new(continent_def_tu0)
            .set_seed(seed + 11)
            .set_frequency(CONTINENT_FREQUENCY * 47.25)
            .set_power(CONTINENT_FREQUENCY / 433.75)
            .set_roughness(12);

        // 3: [Warped-base-continent-definition module]: This turbulence module
        // warps the output value from the intermediate-turbulence module. This
        // turbulence has a higher frequency, but lower power, than the
        // intermediate-turbulence module, adding some fine detail to it.
        let continent_def_tu2 = Turbulence::<_, Perlin>::new(continent_def_tu1)
            .set_seed(seed + 12)
            .set_frequency(CONTINENT_FREQUENCY * 95.25)
            .set_power(CONTINENT_FREQUENCY / 1019.75)
            .set_roughness(11);

        // 4: [Select-turbulence module]: At this stage, the turbulence is applied
        // to the entire base-continent-definition subgroup, producing some very
        // rugged, unrealistic coastlines.  This selector module selects the
        // output values from the (unwarped) base-continent-definition subgroup
        // and the warped-base-continent-definition module, based on the output
        // value from the (unwarped) base-continent-definition subgroup.  The
        // selection boundary is near sea level and has a relatively smooth
        // transition.  In effect, only the higher areas of the base-continent-
        // definition subgroup become warped; the underwater and coastal areas
        // remain unaffected.
        let continent_def_se = Select::new(
            base_continent_def(seed),
            continent_def_tu2,
            base_continent_def(seed),
        )
        .set_bounds(SEA_LEVEL - 0.0375, SEA_LEVEL + 1000.0375)
        .set_falloff(0.0625);

        let continent_def = Cache::new(continent_def_se);

        continent_def
    }

    continent_tu_def(seed)
}
