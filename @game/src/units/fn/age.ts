const units: [number, string][] = [[1e9, "Gyr"], [1e6, "Myr"], [1e3, "kyr"]];
const scales: Record<string, number> = { "": 1, y: 1, yr: 1, k: 1e3, kyr: 1e3, ka: 1e3, m: 1e6, myr: 1e6, ma: 1e6, g: 1e9, gyr: 1e9, ga: 1e9 };

export function formatAge(years: number) {
  for (const [scale, unit] of units) {
    if (years >= scale) return `${Number((years / scale).toPrecision(4))} ${unit}`;
  }
  return `${Math.round(years)} yr`;
}

// Years from "4.6 Gyr", "250 Myr", "1e9" or a plain count, where commas may only group thousands. NaN when the
// text is not a finite, non-negative age.
export function parseAge(text: string) {
  const match = /^(\d{1,3}(?:,\d{3})+|\d*\.?\d+(?:e[+-]?\d+)?)([a-z]*)$/i.exec(text.replaceAll(/[_\s]/g, ""));
  const unit = match?.[2].toLowerCase() ?? "";
  if (!match || !Object.hasOwn(scales, unit)) return Number.NaN;
  const years = Math.round(Number(match[1].replaceAll(",", "")) * scales[unit]);
  return Number.isFinite(years) ? years : Number.NaN;
}
