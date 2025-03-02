/** @format */

import { HSLA, type Biome } from "@derivean/utils";

export const TemperatureBiome: Biome = {
	type: "Temperature",
	resolve({ color, source }) {
		// Get the temperature noise value
		const temperatureValue = source.temperature;

		// Extract current color values
		const [, , lightness, alpha] = color.color;

		// Convert temperature value from [-1, 1] to [0, 1]
		const normalizedTemperature = (temperatureValue + 1) / 2;

		// Define hue based on temperature:
		// Cold: Blue (210°), Neutral: Purple/Green (140°), Hot: Red/Orange (20°)
		const hue = 210 - normalizedTemperature * 190;

		// Calculate saturation based on how extreme the temperature is
		// More extreme temperatures (very hot or very cold) get more saturation
		const temperatureExtreme = Math.abs(temperatureValue * 2 - 1);
		const saturation = 15 + 25 * temperatureExtreme;

		// Apply temperature coloring but preserve original lightness
		return { color: HSLA([hue, saturation, lightness, alpha]), exclusive: false };
	},
};
