/** @format */

import { ComplexBiome } from "@derivean/terrain";
import { type GameConfig } from "@derivean/utils";

export const gameConfig: GameConfig = {
	maxZoom: 1,
	minZoom: 0.005,
	plotSize: 16,
	plotCount: 256,
	chunkSize: 16 * 256,
	chunkLimit: 2048,
	biomes: [ComplexBiome],
	layers: [
		{ min: 0.005, max: 0.0035, level: 64, offset: 4 },
		{ min: 0.0035, max: 0.005, level: 16, offset: 4 },
		{ min: 0.005, max: 0.015, level: 8, offset: 4 },
		{ min: 0.015, max: 0.025, level: 4, offset: 4 },
		{ min: 0.025, max: 0.04, level: 2, offset: 4 },
		{ min: 0.04, max: 1, level: 1, offset: 4 },
	],
};
