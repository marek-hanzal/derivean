use noise::{
    Add, Billow, Blend, Cache, Clamp, Constant, Curve, Exponent, Fbm, Max, Min, MultiFractal,
    Multiply, NoiseFn, Perlin, RidgedMulti, ScaleBias, Seedable, Select, Terrace, Turbulence,
    Worley, core::worley::ReturnType,
};
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
    base: Box<dyn NoiseFn<f64, 3>>,
    perlin: Fbm<Perlin>,
}

const CONTINENT_FREQUENCY: f64 = 1.;
const CONTINENT_LACUNARITY: f64 = 2.208984375;
const SEA_LEVEL: f64 = 0.;
const TERRAIN_OFFSET: f64 = 1.0;
const SHELF_LEVEL: f64 = -0.375;
const MOUNTAIN_LACUNARITY: f64 = 2.142578125;
const MOUNTAINS_TWIST: f64 = 1.0;
const MOUNTAIN_GLACIATION: f64 = 1.375;
const MOUNTAINS_AMOUNT: f64 = 0.5;
const HILLS_TWIST: f64 = 1.0;
const HILLS_LACUNARITY: f64 = 2.162109375;
const PLAINS_LACUNARITY: f64 = 2.314453125;
const BADLANDS_LACUNARITY: f64 = 2.212890625;
const BADLANDS_TWIST: f64 = 1.0;
const CONTINENT_HEIGHT_SCALE: f64 = (1.0 - SEA_LEVEL) / 4.0;
const HILLS_AMOUNT: f64 = (1.0 + MOUNTAINS_AMOUNT) / 2.0;
const BADLANDS_AMOUNT: f64 = 0.3125;
const RIVER_DEPTH: f64 = 0.0234375;

fn base_continent_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let base_continent_def_fb0 = Fbm::<Perlin>::new(seed)
        .set_frequency(CONTINENT_FREQUENCY)
        .set_persistence(0.5)
        .set_lacunarity(CONTINENT_LACUNARITY)
        .set_octaves(14);

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

    let base_continent_def_fb1 = Fbm::<Perlin>::new(seed + 1)
        .set_frequency(CONTINENT_FREQUENCY * 4.34375)
        .set_persistence(0.5)
        .set_lacunarity(CONTINENT_LACUNARITY)
        .set_octaves(11);

    let base_continent_def_sb = ScaleBias::new(base_continent_def_fb1)
        .set_scale(0.375)
        .set_bias(0.625);

    let base_continent_def_mi = Min::new(base_continent_def_sb, base_continent_def_cu);

    let base_continent_def_cl = Clamp::new(base_continent_def_mi).set_bounds(-1.0, 1.0);

    let base_continent_def = Cache::new(base_continent_def_cl);

    base_continent_def
}

fn continent_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continent_def_tu0 = Turbulence::<_, Perlin>::new(base_continent_def(seed))
        .set_seed(seed + 10)
        .set_frequency(CONTINENT_FREQUENCY * 15.25)
        .set_power(CONTINENT_FREQUENCY / 113.75)
        .set_roughness(13);

    let continent_def_tu1 = Turbulence::<_, Perlin>::new(continent_def_tu0)
        .set_seed(seed + 11)
        .set_frequency(CONTINENT_FREQUENCY * 47.25)
        .set_power(CONTINENT_FREQUENCY / 433.75)
        .set_roughness(12);

    let continent_def_tu2 = Turbulence::<_, Perlin>::new(continent_def_tu1)
        .set_seed(seed + 12)
        .set_frequency(CONTINENT_FREQUENCY * 95.25)
        .set_power(CONTINENT_FREQUENCY / 1019.75)
        .set_roughness(11);

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

fn terrain_type_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let terrain_type_def_tu = Turbulence::<_, Perlin>::new(continent_def(seed))
        .set_seed(seed + 20)
        .set_frequency(CONTINENT_FREQUENCY * 18.125)
        .set_power(CONTINENT_FREQUENCY / 20.59375 * TERRAIN_OFFSET)
        .set_roughness(3);

    let terrain_type_def_te = Terrace::new(terrain_type_def_tu)
        .add_control_point(-1.00)
        .add_control_point(SHELF_LEVEL + SEA_LEVEL / 2.0)
        .add_control_point(1.00);

    let terrain_type_def = Cache::new(terrain_type_def_te);

    terrain_type_def
}

fn mountain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let mountain_base_def_rm0 = RidgedMulti::<Perlin>::new(seed + 30)
        .set_frequency(1723.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(4);

    let mountain_base_def_sb0 = ScaleBias::new(mountain_base_def_rm0)
        .set_scale(0.5)
        .set_bias(0.375);

    let mountain_base_def_rm1 = RidgedMulti::<Perlin>::new(seed + 31)
        .set_frequency(367.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(1);

    let mountain_base_def_sb1 = ScaleBias::new(mountain_base_def_rm1)
        .set_scale(-2.0)
        .set_bias(-0.5);

    let mountain_base_def_co = Constant::new(-1.0);

    let mountain_base_def_bl = Blend::new(
        mountain_base_def_co,
        mountain_base_def_sb0,
        mountain_base_def_sb1,
    );

    let mountain_base_def_tu0 = Turbulence::<_, Perlin>::new(mountain_base_def_bl)
        .set_seed(seed + 32)
        .set_frequency(1337.0)
        .set_power(1.0 / 6730.0 * MOUNTAINS_TWIST)
        .set_roughness(4);

    let mountain_base_def_tu1 = Turbulence::<_, Perlin>::new(mountain_base_def_tu0)
        .set_seed(seed + 33)
        .set_frequency(21221.0)
        .set_power(1.0 / 120157.0 * MOUNTAINS_TWIST)
        .set_roughness(6);

    let mountain_base_def = Cache::new(mountain_base_def_tu1);

    mountain_base_def
}

fn mountain_high_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let mountainous_high_rm0 = RidgedMulti::<Perlin>::new(seed + 40)
        .set_frequency(2371.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(3);

    let mountainous_high_rm1 = RidgedMulti::<Perlin>::new(seed + 41)
        .set_frequency(2341.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(3);

    let mountainous_high_ma = Max::new(mountainous_high_rm0, mountainous_high_rm1);

    let mountainous_high_tu = Turbulence::<_, Perlin>::new(mountainous_high_ma)
        .set_seed(seed + 42)
        .set_frequency(31511.0)
        .set_power(1.0 / 180371.0 * MOUNTAINS_TWIST)
        .set_roughness(4);

    let mountainous_high = Cache::new(mountainous_high_tu);

    mountainous_high
}

fn mountain_low_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let mountainous_low_rm0 = RidgedMulti::<Perlin>::new(seed + 50)
        .set_frequency(1381.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(8);

    let mountainous_low_rm1 = RidgedMulti::<Perlin>::new(seed + 51)
        .set_frequency(1427.0)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(8);

    let mountainous_low_mu = Multiply::new(mountainous_low_rm0, mountainous_low_rm1);

    let mountainous_low = Cache::new(mountainous_low_mu);

    mountainous_low
}

fn mountain_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let mountain_low = mountain_low_def(seed);
    let mountain_high = mountain_high_def(seed);

    let mountainous_terrain_sb0 = ScaleBias::new(mountain_low)
        .set_scale(0.03125)
        .set_bias(-0.96875);

    let mountainous_terrain_sb1 = ScaleBias::new(mountain_high).set_scale(0.25).set_bias(0.25);

    let mountainous_terrain_ad = Add::new(mountainous_terrain_sb1, mountain_def(seed));

    let mountainous_terrain_se = Select::new(
        mountainous_terrain_sb0,
        mountainous_terrain_ad,
        mountain_def(seed),
    )
    .set_bounds(-0.5, 999.5)
    .set_falloff(0.5);

    let mountainous_terrain_sb2 = ScaleBias::new(mountainous_terrain_se)
        .set_scale(0.8)
        .set_bias(0.0);
    let mountainous_terrain_ex =
        Exponent::new(mountainous_terrain_sb2).set_exponent(MOUNTAIN_GLACIATION);

    let mountainous_terrain = Cache::new(mountainous_terrain_ex);

    mountainous_terrain
}

fn hilly_terrain(seed: u32) -> impl NoiseFn<f64, 3> {
    let hilly_terrain_bi = Billow::<Perlin>::new(seed + 60)
        .set_frequency(1663.0)
        .set_persistence(0.5)
        .set_lacunarity(HILLS_LACUNARITY)
        .set_octaves(6);

    let hilly_terrain_sb0 = ScaleBias::new(hilly_terrain_bi)
        .set_scale(0.5)
        .set_bias(0.5);

    let hilly_terrain_rm = RidgedMulti::<Perlin>::new(seed + 61)
        .set_frequency(367.5)
        .set_lacunarity(HILLS_LACUNARITY)
        .set_octaves(1);

    let hilly_terrain_sb1 = ScaleBias::new(hilly_terrain_rm)
        .set_scale(-2.0)
        .set_bias(-1.0);

    let hilly_terrain_co = Constant::new(-1.0);

    let hilly_terrain_bl = Blend::new(hilly_terrain_co, hilly_terrain_sb1, hilly_terrain_sb0);

    let hilly_terrain_sb2 = ScaleBias::new(hilly_terrain_bl)
        .set_scale(0.75)
        .set_bias(-0.25);

    let hilly_terrain_ex = Exponent::new(hilly_terrain_sb2).set_exponent(1.375);

    let hilly_terrain_tu0 = Turbulence::<_, Perlin>::new(hilly_terrain_ex)
        .set_seed(seed + 62)
        .set_frequency(1531.0)
        .set_power(1.0 / 16921.0 * HILLS_TWIST)
        .set_roughness(4);

    let hilly_terrain_tu1 = Turbulence::<_, Perlin>::new(hilly_terrain_tu0)
        .set_seed(seed + 63)
        .set_frequency(21617.0)
        .set_power(1.0 / 117529.0 * HILLS_TWIST)
        .set_roughness(6);

    let hilly_terrain = Cache::new(hilly_terrain_tu1);

    hilly_terrain
}

fn plain_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let plains_terrain_bi0 = Billow::<Perlin>::new(seed + 70)
        .set_frequency(1097.5)
        .set_persistence(0.5)
        .set_lacunarity(PLAINS_LACUNARITY)
        .set_octaves(8);

    let plains_terrain_sb0 = ScaleBias::new(plains_terrain_bi0)
        .set_scale(0.5)
        .set_bias(0.5);

    let plains_terrain_bi1 = Billow::<Perlin>::new(seed + 71)
        .set_frequency(1097.5)
        .set_persistence(0.5)
        .set_lacunarity(PLAINS_LACUNARITY)
        .set_octaves(8);

    let plains_terrain_sb1 = ScaleBias::new(plains_terrain_bi1)
        .set_scale(0.5)
        .set_bias(0.5);

    let plains_terrain_mu = Multiply::new(plains_terrain_sb0, plains_terrain_sb1);

    let plains_terrain_sb2 = ScaleBias::new(plains_terrain_mu)
        .set_scale(2.0)
        .set_bias(-1.0);

    let plains_terrain = Cache::new(plains_terrain_sb2);

    plains_terrain
}

fn badlands_sand_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let badlands_sand_rm = RidgedMulti::<Perlin>::new(seed + 80)
        .set_frequency(6163.5)
        .set_lacunarity(BADLANDS_LACUNARITY)
        .set_octaves(1);

    let badlands_sand_sb0 = ScaleBias::new(badlands_sand_rm)
        .set_scale(0.875)
        .set_bias(0.0);

    let badlands_sand_wo = Worley::new(seed + 81)
        .set_frequency(16183.25)
        .set_return_type(ReturnType::Distance);

    let badlands_sand_sb1 = ScaleBias::new(badlands_sand_wo)
        .set_scale(0.25)
        .set_bias(0.25);

    let badlands_sand_ad = Add::new(badlands_sand_sb0, badlands_sand_sb1);

    let badlands_sand = Cache::new(badlands_sand_ad);

    badlands_sand
}

fn badlands_cliffs_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let badlands_cliffs_fb = Fbm::<Perlin>::new(seed + 90)
        .set_frequency(CONTINENT_FREQUENCY * 839.0)
        .set_persistence(0.5)
        .set_lacunarity(BADLANDS_LACUNARITY)
        .set_octaves(6);

    let badlands_cliffs_cu = Curve::new(badlands_cliffs_fb)
        .add_control_point(-2.000, -2.000)
        .add_control_point(-1.000, -1.000)
        .add_control_point(-0.000, -0.750)
        .add_control_point(0.500, -0.250)
        .add_control_point(0.625, 0.875)
        .add_control_point(0.750, 1.000)
        .add_control_point(2.000, 1.250);

    let badlands_cliffs_cl = Clamp::new(badlands_cliffs_cu).set_bounds(-999.125, 0.875);

    let badlands_cliffs_te = Terrace::new(badlands_cliffs_cl)
        .add_control_point(-1.000)
        .add_control_point(-0.875)
        .add_control_point(-0.750)
        .add_control_point(-0.500)
        .add_control_point(0.000)
        .add_control_point(1.000);

    let badlands_cliffs_tu0 = Turbulence::<_, Perlin>::new(badlands_cliffs_te)
        .set_seed(seed + 91)
        .set_frequency(16111.0)
        .set_power(1.0 / 141539.0 * BADLANDS_TWIST)
        .set_roughness(3);

    let badlands_cliffs_tu1 = Turbulence::<_, Perlin>::new(badlands_cliffs_tu0)
        .set_seed(seed + 92)
        .set_frequency(36107.0)
        .set_power(1.0 / 211543.0 * BADLANDS_TWIST)
        .set_roughness(3);

    let badlands_cliffs = Cache::new(badlands_cliffs_tu1);

    badlands_cliffs
}

fn badlands_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let badlands_sand = badlands_sand_def(seed);
    let badlands_cliffs = badlands_cliffs_def(seed);

    let badlands_terrain_sb = ScaleBias::new(badlands_sand)
        .set_scale(0.25)
        .set_bias(-0.75);

    let badlands_terrain_ma = Max::new(badlands_cliffs, badlands_terrain_sb);

    let badlands_terrain = Cache::new(badlands_terrain_ma);

    badlands_terrain
}

fn river_positions_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let river_positions_rm0 = RidgedMulti::<Perlin>::new(seed + 100)
        .set_frequency(18.75)
        .set_lacunarity(CONTINENT_LACUNARITY)
        .set_octaves(1);

    let river_positions_cu0 = Curve::new(river_positions_rm0)
        .add_control_point(-2.000, 2.000)
        .add_control_point(-1.000, 1.000)
        .add_control_point(-0.125, 0.875)
        .add_control_point(0.000, -1.000)
        .add_control_point(1.000, -1.500)
        .add_control_point(2.000, -2.000);

    let river_positions_rm1 = RidgedMulti::<Perlin>::new(seed + 101)
        .set_frequency(43.25)
        .set_lacunarity(CONTINENT_LACUNARITY)
        .set_octaves(1);

    let river_positions_cu1 = Curve::new(river_positions_rm1)
        .add_control_point(-2.000, 2.0000)
        .add_control_point(-1.000, 1.5000)
        .add_control_point(-0.125, 1.4375)
        .add_control_point(0.000, 0.5000)
        .add_control_point(1.000, 0.2500)
        .add_control_point(2.000, 0.0000);

    let river_positions_mi = Min::new(river_positions_cu0, river_positions_cu1);

    let river_positions_tu = Turbulence::<_, Perlin>::new(river_positions_mi)
        .set_seed(seed + 102)
        .set_frequency(9.25)
        .set_power(1.0 / 57.75)
        .set_roughness(6);

    let river_positions = Cache::new(river_positions_tu);

    river_positions
}

fn scaled_mountain_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let mountain_terrain = mountain_terrain_def(seed);

    let scaled_mountainous_terrain_sb0 = ScaleBias::new(mountain_terrain)
        .set_scale(0.125)
        .set_bias(0.125);

    let scaled_mountainous_terrain_fb = Fbm::<Perlin>::new(seed + 110)
        .set_frequency(14.5)
        .set_persistence(0.5)
        .set_lacunarity(MOUNTAIN_LACUNARITY)
        .set_octaves(6);

    let scaled_mountainous_terrain_ex =
        Exponent::new(scaled_mountainous_terrain_fb).set_exponent(1.25);

    let scaled_mountainous_terrain_sb1 = ScaleBias::new(scaled_mountainous_terrain_ex)
        .set_scale(0.25)
        .set_bias(1.0);

    let scaled_mountainous_terrain_mu = Multiply::new(
        scaled_mountainous_terrain_sb0,
        scaled_mountainous_terrain_sb1,
    );

    let scaled_mountainous_terrain = Cache::new(scaled_mountainous_terrain_mu);

    scaled_mountainous_terrain
}

fn scaled_hilly_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let hilly_terrain = hilly_terrain(seed);

    let scaled_hilly_terrain_sb0 = ScaleBias::new(hilly_terrain)
        .set_scale(0.0625)
        .set_bias(0.0625);

    let scaled_hilly_terrain_fb = Fbm::<Perlin>::new(seed + 120)
        .set_frequency(13.5)
        .set_persistence(0.5)
        .set_lacunarity(HILLS_LACUNARITY)
        .set_octaves(6);

    let scaled_hilly_terrain_ex = Exponent::new(scaled_hilly_terrain_fb).set_exponent(1.25);

    let scaled_hilly_terrain_sb1 = ScaleBias::new(scaled_hilly_terrain_ex)
        .set_scale(0.5)
        .set_bias(1.5);

    let scaled_hilly_terrain_mu = Multiply::new(scaled_hilly_terrain_sb0, scaled_hilly_terrain_sb1);

    let scaled_hilly_terrain = Cache::new(scaled_hilly_terrain_mu);

    scaled_hilly_terrain
}

fn scaled_plains_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let scaled_plains_terrain_sb0 = ScaleBias::new(plain_terrain_def(seed))
        .set_scale(0.00390625)
        .set_bias(0.0078125);

    let scaled_plains_terrain = Cache::new(scaled_plains_terrain_sb0);

    return scaled_plains_terrain;
}

fn scaled_badlands_terrain_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let scaled_badlands_terrain_sb = ScaleBias::new(badlands_terrain_def(seed))
        .set_scale(0.0625)
        .set_bias(0.0625);

    let scaled_badlands_terrain = Cache::new(scaled_badlands_terrain_sb);

    scaled_badlands_terrain
}

fn continental_shelf_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continental_shelf_te = Terrace::new(continent_def(seed))
        .add_control_point(-1.0)
        .add_control_point(-0.75)
        .add_control_point(SHELF_LEVEL)
        .add_control_point(1.0);

    let continental_shelf_cl = Clamp::new(continental_shelf_te).set_bounds(-0.75, SEA_LEVEL);

    let continental_shelf_rm = RidgedMulti::<Perlin>::new(seed + 130)
        .set_frequency(CONTINENT_FREQUENCY * 4.375)
        .set_lacunarity(CONTINENT_LACUNARITY)
        .set_octaves(16);

    let continental_shelf_sb = ScaleBias::new(continental_shelf_rm)
        .set_scale(-0.125)
        .set_bias(-0.125);

    let continental_shelf_ad = Add::new(continental_shelf_sb, continental_shelf_cl);

    let continental_shelf = Cache::new(continental_shelf_ad);

    continental_shelf
}

fn base_continent_elevation_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let base_continent_elev_sb = ScaleBias::new(continent_def(seed))
        .set_scale(CONTINENT_HEIGHT_SCALE)
        .set_bias(0.0);

    let base_continent_elev_se = Select::new(
        base_continent_elev_sb,
        continental_shelf_def(seed),
        continent_def(seed),
    )
    .set_bounds(SHELF_LEVEL - 1000.0, SHELF_LEVEL)
    .set_falloff(0.03125);

    let base_continent_elev = Cache::new(base_continent_elev_se);

    base_continent_elev
}

fn continents_with_plains_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continents_with_plains_ad = Add::new(
        base_continent_elevation_def(seed),
        scaled_plains_terrain_def(seed),
    );

    let continents_with_plains = Cache::new(continents_with_plains_ad);

    continents_with_plains
}

fn continents_with_hills_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continents_with_hills_ad = Add::new(
        base_continent_elevation_def(seed),
        scaled_hilly_terrain_def(seed),
    );

    let continents_with_hills_se = Select::new(
        continents_with_plains_def(seed),
        continents_with_hills_ad,
        terrain_type_def(seed),
    )
    .set_bounds(1.0 - HILLS_AMOUNT, 1001.0 - HILLS_AMOUNT)
    .set_falloff(0.25);

    let continents_with_hills = Cache::new(continents_with_hills_se);

    continents_with_hills
}

fn continents_with_mountains_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continents_with_mountains_ad0 = Add::new(
        base_continent_elevation_def(seed),
        scaled_mountain_terrain_def(seed),
    );

    let continents_with_mountains_cu = Curve::new(continent_def(seed))
        .add_control_point(-1.0, -0.0625)
        .add_control_point(0.0, 0.0000)
        .add_control_point(1.0 - MOUNTAINS_AMOUNT, 0.0625)
        .add_control_point(1.0, 0.2500);

    let continents_with_mountains_ad1 =
        Add::new(continents_with_mountains_ad0, continents_with_mountains_cu);

    let continents_with_mountains_se = Select::new(
        continents_with_hills_def(seed),
        continents_with_mountains_ad1,
        terrain_type_def(seed),
    )
    .set_bounds(1.0 - MOUNTAINS_AMOUNT, 1001.0 - MOUNTAINS_AMOUNT)
    .set_falloff(0.25);

    let continents_with_mountains = Cache::new(continents_with_mountains_se);

    continents_with_mountains
}

fn continents_with_badlands_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continents_with_badlands_se = Select::new(
        continents_with_mountains_def(seed),
        Add::new(
            base_continent_elevation_def(seed),
            scaled_badlands_terrain_def(seed),
        ),
        Fbm::<Perlin>::new(seed + 140)
            .set_frequency(16.5)
            .set_persistence(0.5)
            .set_lacunarity(CONTINENT_LACUNARITY)
            .set_octaves(2),
    )
    .set_bounds(1.0 - BADLANDS_AMOUNT, 1001.0 - BADLANDS_AMOUNT)
    .set_falloff(0.25);

    let continents_with_badlands_ma = Max::new(
        continents_with_mountains_def(seed),
        continents_with_badlands_se,
    );

    let continents_with_badlands = Cache::new(continents_with_badlands_ma);

    continents_with_badlands
}

fn continents_with_rivers_def(seed: u32) -> impl NoiseFn<f64, 3> {
    let continents_with_rivers_sb = ScaleBias::new(river_positions_def(seed))
        .set_scale(RIVER_DEPTH / 2.0)
        .set_bias(-RIVER_DEPTH / 2.0);

    let continents_with_rivers_ad = Add::new(
        continents_with_badlands_def(seed),
        continents_with_rivers_sb,
    );

    let continents_with_rivers_se = Select::new(
        continents_with_badlands_def(seed),
        continents_with_rivers_ad,
        continents_with_badlands_def(seed),
    )
    .set_bounds(SEA_LEVEL, CONTINENT_HEIGHT_SCALE + SEA_LEVEL)
    .set_falloff(CONTINENT_HEIGHT_SCALE - SEA_LEVEL);

    let continents_with_rivers = Cache::new(continents_with_rivers_se);

    continents_with_rivers
}

fn generator(seed: u32) -> impl NoiseFn<f64, 3> {
    let unscaled_final_planet = Cache::new(continents_with_rivers_def(seed));

    unscaled_final_planet
}

#[wasm_bindgen]
impl TerrainGenerator {
    #[wasm_bindgen(constructor)]
    pub fn new(seed: String) -> TerrainGenerator {
        let seed = seed_of(&seed);

        TerrainGenerator {
            base: Box::new(generator(seed)),

            perlin: Fbm::<Perlin>::new(seed + 1)
                .set_frequency(1.0 * 4.34375)
                .set_persistence(0.5)
                .set_lacunarity(2.2556543)
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
