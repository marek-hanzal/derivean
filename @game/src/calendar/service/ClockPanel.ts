import type { Frame } from "../../../contract/GameBridge";
import { select } from "../../dom/fn/select";
import { ClockRates, DaySeconds, SeasonSeconds } from "../constant/rates";
import { readClock, readRate } from "../fn/readClock";
import type { WorldClock } from "./WorldClock";

// The clock's face in the HUD and its buttons in the launcher. Keys reach the same clock through `press`.
export function createClockPanel(menu: HTMLElement, hud: HTMLElement, clock: WorldClock) {
  const rates = select(menu, "#clock-rates");
  const skips = select(menu, "#clock-skips");
  const face = select(hud, "#clock");
  const date = select(menu, "#clock-date");

  function button(parent: HTMLElement, label: string, act: () => void) {
    const made = document.createElement("button");
    made.type = "button";
    made.textContent = label;
    made.addEventListener("click", act);
    parent.append(made);
    return made;
  }
  const rateButtons = ClockRates.map((rate) => ({ rate, made: button(rates, rate === 0 ? "Hold" : readRate(rate), () => { clock.rate = rate; }) }));
  button(skips, "+1 day", () => clock.skip(DaySeconds));
  button(skips, "+1 season", () => clock.skip(SeasonSeconds));

  return {
    // True when the key was one of the clock's.
    press(code: string) {
      if (code === "KeyP") clock.toggle();
      else if (code === "Comma") clock.shift(-1);
      else if (code === "Period") clock.shift(1);
      else if (code === "KeyN") clock.skip(DaySeconds);
      else if (code === "KeyB") clock.skip(SeasonSeconds);
      else return false;
      return true;
    },
    paint(calendar: Frame["calendar"]) {
      const text = `${readClock(calendar)} · ${readRate(clock.rate)}`;
      if (face.textContent !== text) face.textContent = date.textContent = text;
      for (const { rate, made } of rateButtons) made.setAttribute("aria-pressed", String(rate === clock.rate));
    },
  };
}
