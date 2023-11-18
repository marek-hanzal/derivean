export const twAlignItems = {
    "baseline": "items-baseline",
    "center":   "items-center",
} as const;
export type twAlignItems = typeof twAlignItems;
export namespace twAlignItems {
    export type Values = keyof twAlignItems;
}
