export type Point = [number, number, number];
export type BrushShape = "sphere" | "cube";

export interface WorldInfo {
  voxelSize: number;
  chunkMeters: number;
  defaultBrushRadius: number;
  maxBrushRadius: number;
  waterUnitsPerVoxel: number;
  materials: {
    name: string;
    color: number;
    roughness: number;
    density: number;
    support: number;
    friction: number;
    matter: "gas" | "solid" | "granular" | "liquid";
    angleOfRepose: number;
    dynamicViscosity: number;
    surfaceTension: number;
  }[];
}

export interface Command {
  elapsed: number;
  x: number;
  z: number;
  jump: boolean;
  sprint: boolean;
  active: boolean;
  direction: Point;
  radius: number;
  shape: BrushShape;
  dig: boolean;
  place: boolean;
}

export interface ChunkMesh {
  key: Point;
  origin: Point;
  faces: Int16Array;
}

export interface Frame {
  eye: Point;
  target: { voxel: Point; material: number } | null;
  chunks: ChunkMesh[];
  waterMeshes: ChunkMesh[];
  water: { initial: number; bulk: number; airborne: number; escaped: number };
  waterDepth: number | null;
  // x, y, z, cube size in metres, particle kind (water, sand, foam).
  particles: Float32Array;
  unloaded: Point[];
  spawned: { id: number; faces: Int16Array }[];
  debris: { id: number; position: Point; rotation: [number, number, number, number] }[];
  retired: number[];
  editedTiles: number;
  visibleChunks: number;
  bodies: number;
  nativeMs: number;
  buildStock: number;
  streamingReady: boolean;
  sandAirborne: number;
  sandDeposited: number;
  sandEscaped: number;
  debrisEscapedKg: number;
  excavatedKg: number;
}

export interface GameBridge {
  startWorld(): Promise<WorldInfo>;
  advance(command: Command): Promise<Frame>;
  quit(): Promise<void>;
}

declare global {
  interface Window {
    derivean: GameBridge;
  }
}
