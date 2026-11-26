// Calendar seconds per second of play the clock can run at: held, real time, a minute, an hour and a day a second.
export const ClockRates = [0, 1, 60, 3600, 86400] as const;

export const DaySeconds = 86400;
// A quarter of the year, the stretch a "skip a season" covers.
export const SeasonSeconds = 365 * DaySeconds / 4;
