import { fpClamp, fpCombineNoise, fpScaleXZ, fpWeight } from "@derivean/utils";
import { flow } from "fp-ts/lib/function";
import { blend } from "../utils/blend";
import { createNoise } from "../utils/createNoise";

export const heightmap = (seed: string) =>
	flow(
		fpScaleXZ({ scale: 1 }),
		flow(
			fpCombineNoise({
				noise1: {
					noise: blend({
						scale: [2, 5],
						limit: [0.2, 0.8],
						sourceNoise: createNoise({
							seed: `${seed}-heightmap-1`,
							frequency: 0.01,
							type: "Value",
							fractal: {
								type: "FBm",
								gain: 0.25,
								lacunarity: 4,
								octaves: 28,
								weightedStrength: 2.5,
							},
						}),
						controlNoise: createNoise({
							seed: `${seed}-heightmap-2`,
							frequency: 0.01,
							type: "ValueCubic",
							fractal: {
								type: "FBm",
								gain: 0.5,
								lacunarity: 2.5,
								octaves: 64,
								weightedStrength: 0.5,
							},
						}),
					}),
					weight: 1,
				},
				noise2: {
					noise: flow(
						createNoise({
							seed: `${seed}-heightmap-3`,
							frequency: 0.005,
							type: "ValueCubic",
							fractal: {
								type: "FBm",
								gain: 0.75,
								lacunarity: 2.5,
								octaves: 32,
								weightedStrength: 0.5,
							},
						}),
						fpClamp({ min: -1, max: 0 }),
					),
					weight: 1,
				},
			}),
		),
		fpWeight({ weight: 1.5 }),
		fpClamp({ min: -1, max: 1 }),
	);
