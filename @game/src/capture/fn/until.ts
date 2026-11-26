import type { Frame } from "../../../contract/GameBridge";
import { DaySeconds } from "../../calendar/constant/rates";

// Calendar seconds past a moment within which a shot asking for it is already there.
const Reached = 900;

// Seconds from a calendar moment to the next time the given day of the year and hour come round.
export function until(calendar: Frame["calendar"], day: number | undefined, hour: number | undefined) {
  const days = day === undefined ? 0 : (((day - Math.floor(calendar.dayOfYear)) % 365) + 365) % 365;
  let seconds = days * DaySeconds + (hour === undefined ? 0 : (hour - calendar.hour) * 3600);
  // A moment just reached is that moment still, not the same hour a day or a year on.
  if (seconds > -Reached && seconds <= 0) return 0;
  if (seconds < 0) seconds += day === undefined ? DaySeconds : 365 * DaySeconds;
  return seconds;
}
