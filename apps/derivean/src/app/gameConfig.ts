/** @format */

import type { GameConfig } from "@derivean/utils";

export const gameConfig: GameConfig = {
	maxZoom: 1,
	minZoom: 0.005,
	plotSize: 16,
	plotCount: 256,
	chunkSize: 16 * 256,
	chunkLimit: 2048,
	layers: [
		{ min: 0.005, max: 0.015, level: 64, offset: 4 }, // Furthest zoom out (most detail reduction)
		{ min: 0.015, max: 0.025, level: 16, offset: 4 }, // Medium-far distance
		{ min: 0.025, max: 0.04, level: 8, offset: 4 }, // Medium distance
		{ min: 0.04, max: 0.2, level: 4, offset: 4 }, // Medium-close distance
		{ min: 0.2, max: 0.5, level: 2, offset: 4 }, // Close distance
		{ min: 0.5, max: 1, level: 1, offset: 4 }, // Closest zoom in (no detail reduction)
	],
};
