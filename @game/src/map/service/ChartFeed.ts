import type { Chart, GameBridge } from "../../../contract/GameBridge";
import { ChartLevels } from "../constant/chart";
import { ChartDrift, PaneCount, PaneSize, PreviewSize, SharpWorthMs, StaleMs } from "../constant/map";
import { chartLook, measurePane } from "../fn/chartLook";
import { serves } from "../fn/view";
import type { ChartLook, PaneMeasure } from "../type/ChartLook";
import type { ChartPoint } from "../type/ChartPoint";
import type { Square } from "../type/MapView";
import type { ChartSurface } from "./ChartSurface";

// The chart on screen: `count`² panes over `square`, row-major from the north-west.
interface Shown {
  square: Square;
  panes: Chart[];
  count: number;
  sharp: boolean;
  look: ChartLook;
  // The fewest finished tiles any pane was read with: a later count means some pane may be out of date.
  tiles: number;
}

// What a round of requests brings: a coarse chart and then the sharp one, the sharp one alone, or the sharp one
// if the simulation has finished tiles since the chart on screen.
type Round = "fresh" | "sharp" | "news";

// Keeps a chart on the surface for whatever square the map wants. A view the chart on screen no longer serves
// gets a coarse chart at once and the sharp one after it, unless it moves too fast for a sharp one to be worth
// reading; a view that stays put is read again whenever the simulation has finished more tiles. One round of
// requests is in flight at a time.
export function createChartFeed(bridge: Pick<GameBridge, "chart">, surface: ChartSurface, failed: (error: unknown) => void) {
  let shown: Shown | undefined;
  let wanted: Square | undefined;
  let busy = false;
  // Bumped when the world changes: answers that were asked of an earlier one are dropped.
  let world = 0;
  let asked = -Infinity;
  // Where the wanted square was a frame ago, and how fast its centre moves, in metres per millisecond.
  let before: { x: number; z: number; time: number } | undefined;
  let speed = 0;
  let fresh = false;
  let readMs = 0;
  // Charts brought on screen so far, coarse and sharp, for the capture record.
  const shows = { preview: 0, sharp: 0 };

  // Reads `count`² panes of `size` pixels over the square into the surface's next sheet. Each pane is handed
  // on and measured as it arrives, so no frame carries more than one.
  async function read(square: Square, count: number, size: number, mine: number) {
    const span = square.span / count;
    const measures: PaneMeasure[] = [];
    surface.begin(count * size);
    const panes = await Promise.all(Array.from({ length: count * count }, async (_, index) => {
      const column = index % count;
      const row = Math.floor(index / count);
      const pane = await bridge.chart({ x: square.x + (column - (count - 1) / 2) * span, z: square.z + (row - (count - 1) / 2) * span, span, size });
      if (!pane || mine !== world) return null;
      surface.put(pane, column * size, row * size);
      measures.push(measurePane(pane));
      return pane;
    }));
    return panes.every((pane) => pane !== null) ? { panes: panes as Chart[], look: chartLook(measures) } : undefined;
  }

  function show(square: Square, { panes, look }: { panes: Chart[]; look: ChartLook }, count: number, sharp: boolean) {
    surface.show({ origin: panes[0].origin, pitch: panes[0].pitch, markPlanet: panes[0].finest > 0, look });
    shown = { square, panes, count, sharp, look, tiles: Math.min(...panes.map((pane) => pane.tiles)) };
    fresh = true;
    shows[sharp ? "sharp" : "preview"]++;
  }

  const hurried = (square: Square) => speed * SharpWorthMs > ChartDrift * square.span;

  async function round(square: Square, kind: Round) {
    busy = true;
    const mine = world;
    const began = performance.now();
    try {
      if (kind === "news" && !await bridge.chart({ ...square, size: 1, unless: shown?.tiles })) return;
      if (kind === "fresh") {
        const preview = await read(square, 1, PreviewSize, mine);
        if (!preview) return;
        show(square, preview, 1, false);
        // The view has moved on meanwhile, or is on its way out: the next round starts from where it is then.
        if (!wanted || !serves(square, wanted) || hurried(square)) return;
      }
      const sharp = await read(square, PaneCount, PaneSize, mine);
      if (!sharp) return;
      show(square, sharp, PaneCount, true);
      readMs = performance.now() - began;
    } catch (error) {
      if (mine === world) failed(error);
    } finally {
      busy = false;
    }
  }

  return {
    // Call every frame the map is open with the square it wants charted.
    want(square: Square, time: number) {
      if (before && time > before.time) speed += (Math.hypot(square.x - before.x, square.z - before.z) / (time - before.time) - speed) * 0.2;
      before = { x: square.x, z: square.z, time };
      wanted = square;
      if (busy) return;
      if (!shown || !serves(shown.square, square)) void round(square, "fresh");
      else if (!shown.sharp) {
        if (!hurried(square)) void round(square, "sharp");
      } else if (time - asked >= StaleMs) {
        asked = time;
        void round(square, "news");
      }
    },
    // True once since a new chart came on screen.
    takeFresh() {
      const was = fresh;
      fresh = false;
      return was;
    },
    // m³/yr from which the chart on screen draws a stream.
    streamFloor: () => shown?.look.streamFloor ?? Infinity,
    // True while the chart on screen could have read tiles, so that ground known only from the planet is marked.
    marksPlanet: () => Boolean(shown && shown.panes[0].finest > 0),
    // True when a sharp chart that serves the square is on screen. A newer reading of the same square may be
    // on its way; it replaces the one on screen whole.
    settled: (square: Square) => Boolean(shown?.sharp) && serves(shown!.square, square),
    reset() {
      world++;
      shown = undefined;
      surface.clear();
    },
    // The pixel of the chart on screen that holds a world position.
    at(x: number, z: number): ChartPoint | undefined {
      if (!shown) return undefined;
      const { square, panes, count } = shown;
      const span = square.span / count;
      const column = Math.floor((x - square.x) / span + count / 2);
      const row = Math.floor((z - square.z) / span + count / 2);
      if (column < 0 || row < 0 || column >= count || row >= count) return undefined;
      const pane = panes[row * count + column];
      const within = (value: number) => Math.max(0, Math.min(pane.size - 1, Math.floor(value)));
      const pixel = within((z - pane.origin[1]) / pane.pitch) * pane.size + within((x - pane.origin[0]) / pane.pitch);
      return {
        ground: pane.ground[pixel], water: pane.water[pixel], discharge: pane.discharge[pixel], ice: pane.ice[pixel],
        warmth: pane.warmth[pixel], cover: pane.cover[pixel], bedrock: pane.bedrock[pixel], waters: pane.waters[pixel], level: pane.level[pixel],
      };
    },
    // For a capture record: what is on screen and what it took.
    describe() {
      if (!shown) return undefined;
      const byLevel = [0, 0, 0, 0];
      for (const pane of shown.panes) for (const level of pane.level) byLevel[level]++;
      const pixels = byLevel.reduce((sum, count) => sum + count, 0);
      return {
        sharp: shown.sharp, pixels: Math.sqrt(pixels), pitch: shown.panes[0].pitch, tiles: shown.tiles,
        share: Object.fromEntries(ChartLevels.map((name, level) => [name, Number((byLevel[level] / pixels).toFixed(4))])),
        readMs: Number(readMs.toFixed(1)), shows: { ...shows },
      };
    },
  };
}

export type ChartFeed = ReturnType<typeof createChartFeed>;
