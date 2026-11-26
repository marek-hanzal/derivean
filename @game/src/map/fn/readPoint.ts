import type { WorldInfo } from "../../../contract/GameBridge";
import { DaySeconds } from "../../calendar/constant/rates";
import { distance, metres } from "../../units/fn/length";
import { ChartLevels, ChartWater } from "../constant/chart";
import { BareCover } from "../constant/map";
import type { ChartPoint } from "../type/ChartPoint";

const YearSeconds = 365 * DaySeconds;

function flow(discharge: number) {
  const perSecond = discharge / YearSeconds;
  return `${perSecond >= 100 ? perSecond.toFixed(0) : perSecond.toPrecision(2)} m³/s`;
}

function ground(point: ChartPoint, world: WorldInfo) {
  if (point.level === 0) return "known at planet scale only";
  const rock = world.materials[point.bedrock]?.name.toLowerCase() ?? "rock";
  if (point.ice >= 1) return `${distance(point.ice)} of glacier ice on ${rock}`;
  return point.cover < BareCover[0] ? `bare ${rock}` : `${distance(point.cover)} of cover on ${rock}`;
}

// What the water line says. A pixel's kind is the water at its centre, its flow the largest stream crossing
// it: a pixel the map draws a stream through reads as one even where its centre is dry.
function water(point: ChartPoint, streamFloor: number) {
  const depth = point.water - point.ground;
  const deep = `${depth < 10 ? depth.toFixed(2) : depth.toFixed(0)} m deep`;
  if (point.waters === ChartWater.Sea) return `Sea · ${deep}`;
  if (point.waters === ChartWater.Lake) return `Lake · ${deep}`;
  if (point.waters === ChartWater.Stream) return `Stream · ${deep} · ${flow(point.discharge)}`;
  return point.discharge >= streamFloor ? `Stream · ${flow(point.discharge)}` : "dry";
}

// The lines of the map's readout for one world position; `point` is missing where no chart reaches, and
// `streamFloor` is the flow from which the chart on screen draws a stream.
export function readPoint(x: number, z: number, point: ChartPoint | undefined, world: WorldInfo, streamFloor: number): [string, string][] {
  const position: [string, string] = ["Position", `X ${metres(x)}  ·  Z ${metres(z)}`];
  if (!point) return [position, ["Elevation", "—"], ["Water", "—"], ["Ground", "—"], ["Warmth", "—"], ["Detail", "—"]];
  const above = point.ground - world.seaLevel;
  return [
    position,
    ["Elevation", `${metres(point.ground)} m  ·  ${distance(Math.abs(above))} ${above < 0 ? "below" : "above"} the sea`],
    ["Water", water(point, streamFloor)],
    ["Ground", ground(point, world)],
    ["Warmth", `${point.warmth.toFixed(1)} °C over the year`],
    ["Detail", ChartLevels[point.level]],
  ];
}
