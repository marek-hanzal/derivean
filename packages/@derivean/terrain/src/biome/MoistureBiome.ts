/** @format */

import { HSLA, type Biome } from "@derivean/utils";
import { DefaultTerrainLayers } from "../DefaultTerrainLayers";

export const MoistureBiome: Biome = {
	type: "Moisture",
	resolve({ color, source, base }) {
		// Get the moisture value
		const moistureValue = source.moisture;

		// Extract current color values
		const [, , lightness, alpha] = color.color;

		// Convert moisture value from [-1, 1] to [0, 1]
		const normalizedMoisture = (moistureValue + 1) / 2;

		// Create arrays of terrain type names from DefaultTerrainLayers
		const waterTerrainNames = [
			DefaultTerrainLayers.DeepOcean.name,
			DefaultTerrainLayers.Abyss.name,
			DefaultTerrainLayers.TrenchZone.name,
			DefaultTerrainLayers.Ocean.name,
			DefaultTerrainLayers.ShallowWater.name,
			DefaultTerrainLayers.Reef.name,
			DefaultTerrainLayers.Coast.name,
			DefaultTerrainLayers.Beach.name,
		];

		const deepWaterTerrainNames = [
			DefaultTerrainLayers.DeepOcean.name,
			DefaultTerrainLayers.Abyss.name,
			DefaultTerrainLayers.TrenchZone.name,
		];

		const highlandTerrainNames = [
			DefaultTerrainLayers.Highland.name,
			DefaultTerrainLayers.Plateau.name,
			DefaultTerrainLayers.Mountain.name,
			DefaultTerrainLayers.AlpineSlope.name,
			DefaultTerrainLayers.HighMountain.name,
			DefaultTerrainLayers.MountainPeak.name,
		];

		const midlandTerrainNames = [
			DefaultTerrainLayers.Midland.name,
			DefaultTerrainLayers.Foothills.name,
			DefaultTerrainLayers.Savanna.name,
		];

		const depressionTerrainNames = [
			DefaultTerrainLayers.Valley.name,
			DefaultTerrainLayers.Basin.name,
			DefaultTerrainLayers.Wetland.name,
			DefaultTerrainLayers.Canyon.name,
			DefaultTerrainLayers.Ravine.name,
			DefaultTerrainLayers.Crater.name,
		];

		// Helper function to check if terrain belongs to a category
		const isTerrainType = (typeNames: string[]) => {
			return base.type.some((type) => typeNames.includes(type));
		};

		// Determine terrain category
		const isWater = isTerrainType(waterTerrainNames);
		const isDeepWater = isTerrainType(deepWaterTerrainNames);
		const isHighland = isTerrainType(highlandTerrainNames);
		const isMidland = isTerrainType(midlandTerrainNames);
		const isDepression = isTerrainType(depressionTerrainNames);

		let hue, saturation;

		// Apply different color strategies based on terrain type
		if (isWater) {
			// Water terrain: blues with varying saturation
			// Deep water is more blue (220-240), shallow water more cyan (180-220)
			hue = isDeepWater ? 220 : 195;

			// Adjust saturation based on moisture (more moisture = more intense blue)
			saturation = 40 + normalizedMoisture * 30;

			// Slightly adjust hue based on moisture (wetter areas slightly more vivid blue)
			hue += (normalizedMoisture - 0.5) * 10;
		} else if (isHighland) {
			// Highlands: green to blue gradient for moisture
			hue = 120 + normalizedMoisture * 60;
			saturation = 25 + normalizedMoisture * 35;
		} else if (isMidland) {
			// Midlands: yellow-green to green gradient
			hue = 80 + normalizedMoisture * 40;
			saturation = 30 + normalizedMoisture * 30;
		} else if (isDepression) {
			// Depressions: often wetter, more saturated greens and teals
			hue = 100 + normalizedMoisture * 50;
			saturation = 40 + normalizedMoisture * 30;
		} else {
			// Lowlands and other terrain: yellow to green gradient
			hue = 60 + normalizedMoisture * 60;
			saturation = 30 + normalizedMoisture * 25;
		}

		// Preserve most of the original lightness to maintain heightmap information
		// Apply a subtle lightness adjustment based on moisture (wetter = slightly darker)
		const moistureLightnessAdjustment = (0.5 - normalizedMoisture) * 10;
		const newLightness = Math.max(
			5,
			Math.min(95, lightness * 0.85 + moistureLightnessAdjustment),
		);

		// Always return a color transformation
		return { color: HSLA([hue, saturation, newLightness, alpha]), exclusive: false };
	},
};
