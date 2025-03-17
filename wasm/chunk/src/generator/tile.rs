use wasm_bindgen::prelude::wasm_bindgen;

/**
 * Land defines heighmap of a given type of land.
 *
 * Those noises describes various land types generated on the map.
 *
 * Land in general could be e.g. mountain, hill, plain, etc. and defines an empty shape of the terrain without
 * any features.
 */
#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Land {
    /**
     * Weight is used for land selection and blending.
     */
    pub weight: f64,
    /**
     * Heigh is usually used in conjuction with Bione to compute final color of the tile.
     */
    pub height: f64,
}

/**
 * Biome defines a biome laid over the Land.
 *
 * When a target biome is selected, it's based on the underlying land type, temperature and moisture.
 *
 * Biome could be e.g. grassland, desert, ocean, etc.
 *
 * This struct fills the empty world with features like forests, water, etc.
 */
#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Biome {
    /**
     * Biome strength, generally lower values means weaker biome.
     *
     * Weight is also used for biome blending.
     */
    pub weight: f64,
    /**
     * Biome temperature, generally lower values means colder biome.
     */
    pub temp: f64,
    /**
     * Biome moisture, generally lower values means drier biome.
     */
    pub moisture: f64,
}

/**
 * Individual lands available in the world.
 */
#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Lands {
    pub mountain: Land,
    pub platau: Land,
    pub valley: Land,
    pub hill: Land,
    pub plain: Land,
}

/**
 * Individual biomes available in the world.
 */
#[wasm_bindgen]
#[derive(Clone, Copy)]
pub struct Biomes {
    pub deep_ocean: Biome,
    pub ocean: Biome,
    pub grassland: Biome,
    pub desert: Biome,
    pub vulcanic: Biome,
}

/**
 * Tile contains all the information for game logic, but also all the values
 * used to render the tile (thus a game world).
 */
#[wasm_bindgen]
pub struct Tile {
    /**
     * Lands available in the world.
     */
    pub lands: Lands,
    /**
     * Biomes available in the world.
     */
    pub biomes: Biomes,
}
