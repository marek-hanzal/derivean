/** @format */

import type { Noise, XZ } from "@derivean/utils";

// Function to quantize coordinates to the grid
const quantize = (value: number, resolution: number): number => {
	return Math.floor(value / resolution) * resolution;
};

/**
 * Creates a cached version of any noise function
 * Uses a grid-based approach for caching to allow for effective memory usage
 */
export namespace createNoiseCache {
	export interface Props {
		noise: Noise;
		resolution?: number;
		cacheSize?: number;
	}
}

export function createNoiseCache({
	noise,
	resolution = 0.1,
	cacheSize = 10000,
}: createNoiseCache.Props): Noise {
	// Simple LRU cache implementation
	const cache = new Map<string, number>();
	const keys: string[] = [];

	// The cached noise function
	return ([x, z]: XZ): number => {
		// Quantize coordinates to reduce the number of unique cache entries
		const qx = quantize(x, resolution);
		const qz = quantize(z, resolution);
		const key = `${qx},${qz}`;

		// Check if we have this value cached
		if (cache.has(key)) {
			return cache.get(key)!;
		}

		// Calculate the noise value
		const value = noise([qx, qz]);

		// Manage cache size with simple LRU strategy
		if (keys.length >= cacheSize) {
			const oldestKey = keys.shift()!;
			cache.delete(oldestKey);
		}

		// Store in cache
		keys.push(key);
		cache.set(key, value);

		return value;
	};
}
