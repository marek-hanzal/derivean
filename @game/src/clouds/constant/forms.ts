// What a genus looks like below the size of a weather cell, as three continuous numbers that are blended between
// cells and snapshots like every other channel. The simulation says which genus a layer is; this only says how
// heaped, how broad and how wind-streaked its unresolved structure is.
export interface CloudForm {
  // 0 smooth sheet, about a half cells and rolls, 1 heaped domes with flat bases.
  billow: number;
  // 0 elements a kilometre or two across, 1 structures of tens of kilometres.
  breadth: number;
  // 1 drawn out into fibres along the wind.
  streak: number;
}

// In the order of CloudField's genus codes.
export const CloudForms: readonly CloudForm[] = [
  { billow: 1, breadth: 0, streak: 0 },        // cumulus
  { billow: 1, breadth: 1, streak: 0 },        // cumulonimbus
  { billow: 0, breadth: 1, streak: 0 },        // stratus
  { billow: 0.55, breadth: 0.3, streak: 0 },   // stratocumulus
  { billow: 0, breadth: 1, streak: 0 },        // nimbostratus
  { billow: 0.6, breadth: 0, streak: 0 },      // altocumulus
  { billow: 0, breadth: 1, streak: 0 },        // altostratus
  { billow: 0.15, breadth: 0.6, streak: 1 },   // cirrus
  { billow: 0.6, breadth: 0, streak: 0 },      // cirrocumulus
  { billow: 0, breadth: 1, streak: 0.35 },     // cirrostratus
];

export const GenusNames = [
  "cumulus", "cumulonimbus", "stratus", "stratocumulus", "nimbostratus", "altocumulus", "altostratus", "cirrus",
  "cirrocumulus", "cirrostratus",
] as const;

// Share of a cloud's depth its condensed water fills on average once shaped; turns the simulated optical depth
// into extinction per metre.
export const FillShare = 0.45;
// Extinction per metre above which a march step is opaque anyway.
export const DensestCloud = 0.03;
// Cloud cover below which a cell's optical depth is not divided further: a wisp is not a dense speck.
export const ThinnestCover = 0.08;
