/** @format */

import { HSLA, type Biome } from "@derivean/utils";
import { DefaultTerrainLayers } from "../DefaultTerrainLayers";

export const MoistureBiome: Biome = {
	type: "Moisture",
	resolve({ color, source, base }) {
		// Get the moisture and temperature values
		const moistureValue = source.moisture;
		const temperatureValue = source.temperature;

		// Extract current color values
		const [, , lightness, alpha] = color.color;

		// Convert moisture value from [-1, 1] to [0, 1]
		const normalizedMoisture = (moistureValue + 1) / 2;
		const normalizedTemperature = (temperatureValue + 1) / 2;

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

		let hue, saturation, newLightness;

		if (isHighland) {
			// More subtle mountain coloration
			if (base.type.includes(DefaultTerrainLayers.MountainPeak.name)) {
				// Snowy mountain peaks
				if (normalizedTemperature < 0.3) {
					// Cold, very high peaks: pure white with minimal purple hint
					hue = 250; // Slight purple-blue hint
					saturation = 5 + normalizedMoisture * 5;
					newLightness = 90 + (normalizedMoisture - 0.5) * 5;
				} else {
					// Warmer peaks: predominantly gray with very subtle purple
					hue = 260; // Very subtle purple
					saturation = 5 + normalizedMoisture * 10;
					newLightness = 75 + (normalizedMoisture - 0.5) * 10;
				}
			} else if (base.type.includes(DefaultTerrainLayers.HighMountain.name)) {
				// High mountain slopes
				hue = 250; // Slight purple-gray
				saturation = 5 + normalizedMoisture * 10;
				newLightness = 65 + (normalizedMoisture - 0.5) * 10;
			} else if (base.type.includes(DefaultTerrainLayers.Mountain.name)) {
				// Mountain slopes
				hue = 250; // Minimal purple influence
				saturation = 3 + normalizedMoisture * 7;
				newLightness = 55 + (normalizedMoisture - 0.5) * 10;
			} else {
				// Other highland areas
				hue = 250; // Consistent subtle purple-gray
				saturation = 5 + normalizedMoisture * 10;
				newLightness = 60 + (normalizedMoisture - 0.5) * 10;
			}
		} else if (isWater) {
			// Water terrain: blues with clearer, lighter appearance
			hue = isDeepWater ? 210 : 200;
			saturation = 30 + normalizedMoisture * 15;
			hue += (normalizedMoisture - 0.5) * 5;

			// Preserve water lightness strategy
			const terrainLightnessMultiplier = 1.1;
			const lightnessBoost = 15;
			newLightness = Math.max(
				15,
				Math.min(95, lightness * terrainLightnessMultiplier + lightnessBoost),
			);
		} else if (isMidland) {
			// Midlands: yellow-green gradient
			hue = 70 + normalizedMoisture * 30;
			saturation = 35 + normalizedMoisture * 35;
			newLightness = lightness + (normalizedMoisture - 0.5) * 10;
		} else if (isDepression) {
			// Depressions: green to yellow-green gradient
			hue = 100 - normalizedMoisture * 40;
			saturation = 45 + normalizedMoisture * 35;
			newLightness = lightness + (normalizedMoisture - 0.5) * 10;
		} else {
			// Lowlands: yellow to green gradient
			hue = 60 + normalizedMoisture * 40;
			saturation = 35 + normalizedMoisture * 30;
			newLightness = lightness + (normalizedMoisture - 0.5) * 10;
		}

		// Ensure lightness stays within reasonable bounds
		newLightness = Math.max(15, Math.min(95, newLightness));

		// Always return a color transformation
		return {
			color: HSLA([hue, saturation, newLightness, alpha]),
			exclusive: isHighland && base.type.includes(DefaultTerrainLayers.MountainPeak.name),
		};
	},
};
