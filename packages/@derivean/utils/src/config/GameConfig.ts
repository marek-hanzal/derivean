/** @format */

import type { Chunk } from "../chunk/Chunk";
import type { PlotCount } from "../chunk/PlotCount";
import type { PlotSize } from "../chunk/PlotSize";
import type { NoiseColorMap } from "../noise/NoiseColorMap";
import type { NoiseSource } from "../noise/NoiseSource";
import type { Biome } from "../terrain/Biome";

/**
 * General game configuration used to setup all the internals of the game.
 */
export interface GameConfig {
	/**
	 * Defines, how close camera could be used to zoom in.
	 *
	 * Good starting value is 1.
	 */
	maxZoom: number;
	/**
	 * Defines, how far camera could be used to zoom out.
	 *
	 * Good starting value is 0.005 or something like that. Lowest value is basically zero.
	 *
	 * Be careful with this value, as it may break the game, because you may end up with *a lot* of requested
	 * chunks.
	 *
	 * Also, when you modify this value, keep in mind you should also update {@see GameConfig.layers}.
	 */
	minZoom: number;
	/**
	 * Plot size is the size of a single "pixel", that's a building, road or other entity on a map.
	 *
	 * Be careful, plot size is part of a texture size generated from noise, so making this value bigger is not recommended.
	 */
	plotSize: PlotSize;
	/**
	 * Number of plots in a single chunk. Also defines overall texture size, so **be very careful** when changing this value as
	 * higher ones may fry your GPU and memory.
	 */
	plotCount: PlotCount;
	/**
	 * Pre-computed chunks size (must be plotSize * plotCount).
	 *
	 * Also defines texture size of a single chunk. Keep this number at max of 4096.
	 */
	chunkSize: number;
	/**
	 * Defines number of chunks per layer stored in cache.
	 *
	 * Be careful with this number as high numbers (e.g. 512+) may eat bunch of memory
	 * or even kill the browser tab.
	 */
	chunkLimit: number;
	/**
	 * Source is a factory for all the noises needed to generate the world.
	 */
	source: NoiseSource;
	colorMap: NoiseColorMap;
	/**
	 * Biomes used to colorize the world.
	 *
	 * Order of biomes is extremely important, so if you want to mess up with colors, be careful.
	 */
	biomes?: Biome[];
	/**
	 * Defines which layer is rendered in which zoom level.
	 */
	layers: Chunk.Layer[];
}
