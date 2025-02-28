/** @format */

import { DefaultNoiseSource } from "@derivean/noise";
import { DefaultTerrainLayers, MoistureBiome, withLayerColors } from "@derivean/terrain";
import { type GameConfig } from "@derivean/utils";

export const gameConfig: GameConfig = {
	maxZoom: 1,
	minZoom: 0.001,
	plotSize: 16,
	plotCount: 256,
	chunkSize: 16 * 256,
	chunkLimit: 2048,
	source: DefaultNoiseSource,
	colorMap: withLayerColors([
		DefaultTerrainLayers.DeepOcean,
		DefaultTerrainLayers.Ocean,
		DefaultTerrainLayers.ShallowWater,
		DefaultTerrainLayers.Coast,
		DefaultTerrainLayers.Foam,
		DefaultTerrainLayers.Beach,
		DefaultTerrainLayers.Wetland,
		DefaultTerrainLayers.Lowland,
		DefaultTerrainLayers.Valley,
		DefaultTerrainLayers.Grassland,
		DefaultTerrainLayers.Midland,
		DefaultTerrainLayers.Highland,
		DefaultTerrainLayers.Foothills,
		DefaultTerrainLayers.Mountain,
		DefaultTerrainLayers.HighMountain,
		DefaultTerrainLayers.MountainPeak,
	]),
	biomes: [MoistureBiome],
	layers: [
		{ min: 0.001, max: 0.0025, level: 64, offset: 4 },
		{ min: 0.0025, max: 0.005, level: 16, offset: 4 },
		{ min: 0.005, max: 0.015, level: 8, offset: 4 },
		{ min: 0.015, max: 0.025, level: 4, offset: 4 },
		{ min: 0.025, max: 0.04, level: 2, offset: 4 },
		{ min: 0.04, max: 1, level: 1, offset: 4 },
	],
};
