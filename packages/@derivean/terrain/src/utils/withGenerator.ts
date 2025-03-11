/** @format */

import { type Chunk as WasmChunk } from "@derivean/chunk";
import {
	RGBA,
	type Chunk,
	type Color,
	type GameConfig,
	type Noise,
	type XZ,
} from "@derivean/utils";

/**
 * Converts a noise value (-1 to 1) to a grayscale RGBA color
 *
 * @param noise Noise value from -1 to 1
 * @returns RGBA color with full opacity (alpha = 255)
 */
export function noiseToGrayscale(noise: number): Color.RGBA {
	// Clamp the noise value to ensure it's within -1 to 1
	const clampedNoise = Math.max(-1, Math.min(1, noise));

	// Map the noise from [-1, 1] to [0, 1]
	const normalizedNoise = (clampedNoise + 1) / 2;

	// Map the normalized noise to 0-255 range
	const colorValue = Math.round(normalizedNoise * 255);

	// Return grayscale RGBA (all RGB channels have the same value)
	return RGBA([colorValue, colorValue, colorValue, 255]);
}

export namespace withGenerator {
	export interface Layer {
		level: number;
		noise: Noise;
		biome: Noise;
	}

	export namespace Generator {
		export interface Props {
			x: number;
			z: number;
		}
	}

	export type Generator = (props: Generator.Props) => Chunk.Data;

	export interface Props {
		chunk: WasmChunk;
		gameConfig: GameConfig;
		level: Chunk.View.Level;
	}
}

/**
 * Optimized terrain generator
 * Reduces redundant calculations and improves memory access patterns
 */
export const withGenerator = ({
	chunk,
	gameConfig,
	level,
}: withGenerator.Props): withGenerator.Generator => {
	const baseScale = 1 / (gameConfig.plotCount * (1 / level.layer.level));
	const chunkSize = gameConfig.chunkSize * level.layer.level;
	const offset = (gameConfig.chunkSize * (level.layer.level - 1)) / 2;
	const { plotCount } = gameConfig;
	const size = plotCount * plotCount;

	return ({ x, z }) => {
		// Pre-allocate the buffer once
		const buffer = new Uint8Array(size * 4);

		// Pre-compute world X coordinates for all tiles in a row
		const worldXCoords = new Array(plotCount);
		for (let tileX = 0; tileX < plotCount; tileX++) {
			worldXCoords[tileX] = (x * plotCount + tileX) * baseScale;
		}

		// Pre-compute world Z coordinates for all tiles in a column
		const worldZCoords = new Array(plotCount);
		for (let tileZ = 0; tileZ < plotCount; tileZ++) {
			worldZCoords[tileZ] = (z * plotCount + tileZ) * baseScale;
		}

		// Process all pixels in a single loop
		for (let i = 0; i < size; i++) {
			const tileX = i % plotCount;
			const tileZ = Math.floor(i / plotCount);

			const [wx, wz]: XZ = [worldXCoords[tileX], worldZCoords[tileZ]];

			buffer.set(
				noiseToGrayscale(chunk.tile(wx, wz).height).color,
				((plotCount - 1 - tileZ) * plotCount + tileX) * 4,
			);
		}

		return {
			id: `${x}:${z}:${level.layer.level}`,
			x: x * chunkSize + offset,
			z: z * chunkSize + offset,
			size: chunkSize,
			level: level.layer.level,
			texture: { size: plotCount, data: buffer },
		} satisfies Chunk.Data;
	};
};
