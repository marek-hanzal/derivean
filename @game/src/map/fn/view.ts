import { ChartClosest, ChartMargin, ClosestSpan, PaneCount, PaneSize, SpanRungs } from "../constant/map";
import type { Extent, MapView, Square } from "../type/MapView";

// World X and Z under a viewport pixel. North is up: Z grows down the screen.
export function worldAt(view: MapView, [width, height]: Extent, px: number, py: number): [number, number] {
  return [view.x + (px - width / 2) * view.scale, view.z + (py - height / 2) * view.scale];
}

export function screenOf(view: MapView, [width, height]: Extent, x: number, z: number): [number, number] {
  return [(x - view.x) / view.scale + width / 2, (z - view.z) / view.scale + height / 2];
}

// Metres per pixel between which the shorter side of the viewport shows ClosestSpan and the whole planet.
export function scaleLimits([width, height]: Extent, planetSpan: number): [number, number] {
  const shorter = Math.max(1, Math.min(width, height));
  return [ClosestSpan / shorter, planetSpan / shorter];
}

// Zooms by `factor` (above one is wider) while the world point under the pixel stays under it.
export function zoomAt(view: MapView, extent: Extent, px: number, py: number, factor: number, [closest, widest]: [number, number]): MapView {
  const [x, z] = worldAt(view, extent, px, py);
  const scale = Math.max(closest, Math.min(widest, view.scale * factor));
  return { x: x - (px - extent[0] / 2) * scale, z: z - (py - extent[1] / 2) * scale, scale };
}

// The square a chart is asked for so that it covers the viewport with room to pan. Spans come from a ladder and
// centres from the pixel lattice of their span, so a view that pans or zooms a little reads the same points of
// the world again and its streams and shores hold still from one chart to the next.
export function chartSquare(view: MapView, [width, height]: Extent): Square {
  const span = 2 ** (Math.ceil(Math.log2(Math.max(width, height) * view.scale * ChartMargin) * SpanRungs) / SpanRungs);
  const pitch = span / (PaneCount * PaneSize);
  return { x: Math.round(view.x / pitch) * pitch, z: Math.round(view.z / pitch) * pitch, span };
}

// How far the centre of a view that wants `wanted` may lie from the centre of a chart of `span` before the
// viewport reaches past the chart's edge. A wanted square is at least ChartMargin times the viewport.
export function chartReach(span: number, wanted: number) {
  return (span - wanted / ChartMargin) / 2;
}

// True while a chart read for `shown` still serves a view that wants `wanted`: it covers the viewport and is
// not magnified too far.
export function serves(shown: Square, wanted: Square) {
  const reach = chartReach(shown.span, wanted.span);
  return Math.abs(wanted.x - shown.x) <= reach && Math.abs(wanted.z - shown.z) <= reach && wanted.span >= ChartClosest * shown.span;
}
