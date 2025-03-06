/* tslint:disable */
/* eslint-disable */
export function init(): void;
export class Noise {
  private constructor();
  free(): void;
  biome: number;
  heightmap: number;
  temperature: number;
  moisture: number;
  shade: number;
}
export class TerrainGenerator {
  free(): void;
  constructor(seed: string);
  coord(x: number, z: number): Noise;
}
