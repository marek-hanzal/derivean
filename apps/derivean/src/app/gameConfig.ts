/** @format */

import { ComplexBiome } from "@derivean/terrain";
import { TerrainGenerator } from "@derivean/terrain-wasm";
import { type GameConfig } from "@derivean/utils";

export const gameConfig: GameConfig = {
	maxZoom: 1,
	minZoom: 0.001,
	plotSize: 16,
	plotCount: 256,
	chunkSize: 16 * 256,
	chunkLimit: 2048,
	source({ seed }) {
		const terrain = new TerrainGenerator(seed);

		return {
			coord([x, z]) {
				return terrain.coord(x, z);
			},
			free() {
				terrain.free();
			},
		};
	},
	biomes: [ComplexBiome],
	layers: [
		{ min: 0.001, max: 0.0025, level: 64, offset: 4 },
		{ min: 0.0025, max: 0.005, level: 16, offset: 4 },
		{ min: 0.005, max: 0.015, level: 8, offset: 4 },
		{ min: 0.015, max: 0.025, level: 4, offset: 4 },
		{ min: 0.025, max: 0.04, level: 2, offset: 4 },
		{ min: 0.04, max: 1, level: 1, offset: 4 },
	],
};
