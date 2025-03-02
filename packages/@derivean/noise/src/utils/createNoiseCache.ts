/** @format */

import type { Noise } from "@derivean/utils";

/**
 * Creates a cached version of any noise function with optimizations for large scale usage
 */
export namespace createNoiseCache {
	export interface Props {
		noise: Noise;
	}
}

export function createNoiseCache({ noise }: createNoiseCache.Props): Noise {
	/**
	 * TODO Find some way to cache the noise function
	 */
	return noise;
}
