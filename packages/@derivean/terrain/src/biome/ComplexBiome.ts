/** @format */

import { DefaultTerrainLayers } from "@derivean/terrain";
import { HSLA, type Biome } from "@derivean/utils";

export const ComplexBiome: Biome = {
	type: "Comprehensive",
	resolve({ source, color, base }) {
		// Normalize noise values from [-1, 1] to [0, 1]
		const normalizedHeightmap = (source.heightmap + 1) / 2;
		const normalizedTemperature = (source.temperature + 1) / 2;
		const normalizedMoisture = (source.moisture + 1) / 2;

		// Destructure alpha from current color
		const [, , , alpha] = color.color;

		// Terrain classification based on base terrain type
		const isWater = base.type.some((type) =>
			[
				DefaultTerrainLayers.DeepOcean.name,
				DefaultTerrainLayers.Ocean.name,
				DefaultTerrainLayers.ShallowWater.name,
			].includes(type),
		);

		const isMountain = base.type.some((type) =>
			[
				DefaultTerrainLayers.Mountain.name,
				DefaultTerrainLayers.HighMountain.name,
				DefaultTerrainLayers.MountainPeak.name,
			].includes(type),
		);

		const isLowland = base.type.some((type) =>
			[
				DefaultTerrainLayers.Lowland.name,
				DefaultTerrainLayers.Grassland.name,
				DefaultTerrainLayers.Savanna.name,
			].includes(type),
		);

		// Color generation logic
		let hue: number, saturation: number, lightness: number;

		if (isWater) {
			// Water coloration
			hue = 210; // Blue base
			saturation = 30 + normalizedMoisture * 20;
			lightness = 30 + normalizedMoisture * 10;
		} else if (isMountain) {
			// Mountain coloration
			// Combine temperature and moisture for rock/snow variations
			if (normalizedTemperature < 0.3) {
				// Cold, snowy peaks
				hue = 250; // Slight purple-gray
				saturation = 5;
				lightness = 90 - normalizedMoisture * 10;
			} else {
				// Rocky mountains
				// Use temperature and moisture to create variation
				hue = 250 + (normalizedTemperature - 0.5) * 20; // Subtle hue variation
				saturation = 10 + normalizedMoisture * 10;
				lightness = 60 + normalizedTemperature * 15;
			}
		} else if (isLowland) {
			// Lowland coloration
			// Use moisture and temperature to determine vegetation-like colors
			hue = 120 - normalizedTemperature * 60; // Green to brown spectrum
			saturation = 30 + normalizedMoisture * 40;
			lightness = 40 + normalizedMoisture * 20;
		} else {
			// Default terrain coloration
			hue = 30 + normalizedTemperature * 60; // Warm to cool spectrum
			saturation = 20 + normalizedMoisture * 20;
			lightness = 50 + normalizedHeightmap * 20;
		}

		// Apply some randomness and noise influence
		const noiseInfluence = source.biome * 10;
		hue += noiseInfluence;
		saturation += Math.abs(source.biome) * 5;
		lightness += source.shade * 5;

		// Ensure values are within acceptable ranges
		hue %= 360;
		saturation = Math.max(0, Math.min(100, saturation));
		lightness = Math.max(15, Math.min(90, lightness));

		return {
			color: HSLA([hue, saturation, lightness, alpha]),
			// Make exclusive for mountain peaks to ensure crisp snow/rock appearance
			exclusive: base.type.includes(DefaultTerrainLayers.MountainPeak.name),
		};
	},
};
