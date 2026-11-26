import type { Command } from "../../../contract/GameBridge";
import { ClockRates } from "../constant/rates";

// The player's hold on the world clock: a rate that stays until changed and skips that are asked for once. The
// engine owns the calendar itself; this only says how fast to run it.
export function createWorldClock() {
  let rate: number = 1;
  let resume: number = 1;
  let jump = 0;

  return {
    get rate() { return rate; },
    set rate(value: number) {
      if (value > 0) resume = value;
      rate = value;
    },
    // Holds the clock, or lets it run again at the rate it had.
    toggle() { rate = rate > 0 ? 0 : resume; },
    // One step along ClockRates, slower or faster.
    shift(steps: number) {
      const at = ClockRates.findIndex((known) => known >= rate);
      this.rate = ClockRates[Math.max(0, Math.min(ClockRates.length - 1, (at < 0 ? ClockRates.length - 1 : at) + steps))];
    },
    skip(seconds: number) { jump += seconds; },
    // True while a skip has not been handed to the engine yet.
    get skipping() { return jump !== 0; },
    // What the next command tells the engine; a skip is handed over exactly once.
    take(): Command["time"] {
      const time = { rate, jump };
      jump = 0;
      return time;
    },
  };
}

export type WorldClock = ReturnType<typeof createWorldClock>;
