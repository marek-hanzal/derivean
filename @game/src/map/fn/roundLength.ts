const grouped = new Intl.NumberFormat("en-US", { maximumFractionDigits: 0, useGrouping: true });

// The nearest of 1, 2 and 5 times a power of ten that is at least (`up`) or at most the value.
export function roundLength(value: number, up: boolean) {
  const power = 10 ** Math.floor(Math.log10(value));
  const steps = up ? [1, 2, 5, 10] : [10, 5, 2, 1];
  const step = steps.find((candidate) => up ? candidate * power >= value : candidate * power <= value) ?? 1;
  return step * power;
}

// "500 m", "20 km", "2 000 km".
export function lengthLabel(metres: number) {
  const [value, unit] = metres >= 1000 ? [metres / 1000, "km"] : [metres, "m"];
  return `${grouped.format(value).replaceAll(",", " ")} ${unit}`;
}

// The longest round length that fits `room` pixels at `scale` metres per pixel.
export function scaleBar(scale: number, room: number) {
  const metres = roundLength(scale * room, false);
  return { metres, pixels: metres / scale, label: lengthLabel(metres) };
}

// Metres between contour lines: on a slope of one in ten they stand about eight pixels apart. From a metre,
// which the finest tiles can still tell apart, to a kilometre, beyond which a planet's relief has no lines left.
export function contourInterval(scale: number) {
  return Math.max(1, Math.min(1000, roundLength(0.8 * scale, true)));
}
