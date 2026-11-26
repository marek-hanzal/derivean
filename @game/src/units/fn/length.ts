const grouped = new Intl.NumberFormat("en-US", { minimumFractionDigits: 1, maximumFractionDigits: 1, useGrouping: true });

// Metres with thin-space digit groups: world coordinates run to millions.
export function metres(value: number) {
  return grouped.format(value).replaceAll(",", "\u2009");
}

export function distance(value: number) {
  const size = Math.abs(value);
  return size >= 10_000 ? `${(value / 1000).toFixed(1)} km` : `${value.toFixed(size >= 100 ? 0 : 1)} m`;
}
