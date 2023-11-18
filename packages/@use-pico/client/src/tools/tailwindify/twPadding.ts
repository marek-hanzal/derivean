export const twPadding = {
    p: {
        "xs": "p-1",
        "sm": "p-2",
        "md": "p-3",
        "lg": "p-4",
        "xl": "p-5",
    },
    x: {
        "xs": "px-1",
        "sm": "px-2",
        "md": "px-3",
        "lg": "px-4",
        "xl": "px-5",
    },
    y: {
        "xs": "py-1",
        "sm": "py-2",
        "md": "py-3",
        "lg": "py-4",
        "xl": "py-5",
    },
    t: {
        "xs": "pt-1",
        "sm": "pt-2",
        "md": "pt-3",
        "lg": "pt-4",
        "xl": "pt-5",
    },
    b: {
        "xs": "pb-1",
        "sm": "pb-2",
        "md": "pb-3",
        "lg": "pb-4",
        "xl": "pb-5",
    },
} as const;
export type twPadding = typeof twPadding;
