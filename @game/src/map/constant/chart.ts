// The codes of Chart.waters and the names of the levels in Chart.level, as contract/GameBridge.ts gives them.
export const ChartWater = { Dry: 0, Sea: 1, Lake: 2, Stream: 3 } as const;
export const ChartLevels = ["planet", "region", "local", "site", "ground"] as const;

// The rasters of a chart the map draws from, in the order of their texture units. The first FloatRasters are
// 32-bit floats, the rest bytes.
export const Rasters = ["ground", "water", "discharge", "ice", "warmth", "cover", "waters", "level"] as const;
export const FloatRasters = 6;
