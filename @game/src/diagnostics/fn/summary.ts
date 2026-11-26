export interface Summary { samples: number; mean: number; p50: number; p95: number; p99: number; max: number }

export function summary(series: readonly number[]): Summary {
  if (!series.length) return { samples: 0, mean: 0, p50: 0, p95: 0, p99: 0, max: 0 };
  const sorted = series.slice().sort((a, b) => a - b);
  const at = (share: number) => sorted[Math.max(0, Math.ceil(sorted.length * share) - 1)];
  return {
    samples: sorted.length,
    mean: sorted.reduce((sum, value) => sum + value, 0) / sorted.length,
    p50: at(0.5),
    p95: at(0.95),
    p99: at(0.99),
    max: sorted[sorted.length - 1],
  };
}
