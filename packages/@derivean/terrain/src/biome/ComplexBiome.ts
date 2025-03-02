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
		const normalizedBiome = (source.biome + 1) / 2;
		const normalizedShade = (source.shade + 1) / 2;

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
			// Water coloration with more dynamic variations
			hue = 200 + (normalizedBiome * 40 - 20); // Dynamic blue-green spectrum
			saturation = 40 + normalizedMoisture * 30 + normalizedShade * 20;
			lightness = 30 + normalizedMoisture * 20 - normalizedShade * 10;
		} else if (isMountain) {
			// Mountain coloration with more nuanced variations
			if (normalizedTemperature < 0.3) {
				// Cold, snowy peaks with subtle variations
				hue = 240 + (normalizedBiome * 20 - 10); // Slight purple-blue variation
				saturation = 5 + normalizedShade * 10;
				lightness = 85 - normalizedMoisture * 15;
			} else {
				// Rocky mountains with temperature and noise-driven variations
				hue = 250 + (normalizedTemperature * 30 - 15) + (normalizedBiome * 20 - 10);
				saturation = 15 + normalizedMoisture * 15 + normalizedShade * 10;
				lightness = 55 + normalizedTemperature * 20 - normalizedShade * 10;
			}
		} else if (isLowland) {
			// Lowland coloration with rich, dynamic variations
			hue = 100 - normalizedTemperature * 50 + (normalizedBiome * 30 - 15); // Green to brown spectrum
			saturation = 40 + normalizedMoisture * 40 + normalizedShade * 20;
			lightness = 35 + normalizedMoisture * 25 - normalizedShade * 10;
		} else {
			// Default terrain coloration with complex noise interactions
			hue = 30 + normalizedTemperature * 50 + (normalizedBiome * 30 - 15);
			saturation = 25 + normalizedMoisture * 30 + normalizedShade * 20;
			lightness = 45 + normalizedHeightmap * 25 - normalizedShade * 10;
		}

		// Final noise-driven adjustments
		hue += source.biome * 15;
		saturation += Math.abs(source.biome) * 10;
		lightness += source.shade * 8;

		// Ensure values are within acceptable ranges
		hue %= 360;
		saturation = Math.max(0, Math.min(100, saturation));
		lightness = Math.max(15, Math.min(90, lightness));

		return {
			color: HSLA([hue, saturation, lightness, alpha]),
			// Make exclusive for mountain peaks to ensure crisp appearance
			exclusive: base.type.includes(DefaultTerrainLayers.MountainPeak.name),
		};
	},
};
