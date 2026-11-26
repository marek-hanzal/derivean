import type { Frame } from "../../../contract/GameBridge";

// "Day 173 · 10:30" for the calendar's day of the year and hour.
export function readClock(calendar: Frame["calendar"]) {
  const minutes = Math.floor(calendar.hour * 60);
  const hour = String(Math.floor(minutes / 60) % 24).padStart(2, "0");
  return `Day ${Math.floor(calendar.dayOfYear) + 1} · ${hour}:${String(minutes % 60).padStart(2, "0")}`;
}

export function readRate(rate: number) {
  return rate === 0 ? "held" : `×${rate.toLocaleString("en-US")}`;
}
