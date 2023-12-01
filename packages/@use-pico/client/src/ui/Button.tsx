"use client";

import {
    type ButtonHTMLAttributes,
    type FC,
    type ReactNode
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs":         "text-xs",
    "compact-xs": "text-xs",

    "sm":         "text-sm",
    "compact-sm": "text-sm",

    "md":         "text-md px-1",
    "compact-md": "text-md",

    "lg":         "text-lg px-5 py-2 font-medium",
    "compact-lg": "text-lg font-medium",

    "xl":         "text-xl px-6 py-3 font-medium",
    "compact-xl": "text-xl font-medium",
} as const;
type cssSize = typeof cssSize;

const cssVariant = {
    "primary": `
        transition ease-in-out delay-50 duration-25
        bg-sky-600
        hover:(shadow-md shadow-zinc-500)
    `,
    "subtle":  `
        text-slate-500
        transition ease-in-out delay-50 duration-25
        hover:(bg-zinc-50 text-zinc-700)
    `,
} as const;
type cssVariant = typeof cssVariant;

export namespace Button {
    export type Props =
        ButtonHTMLAttributes<HTMLButtonElement>
        & css.Style
        &
        {
            leftSection?: ReactNode;
            size?: Size;
            variant?: Variant;
        };

    export type Size = keyof cssSize;
    export type Variant = keyof cssVariant;
}

export const Button: FC<Button.Props> = (
    {
        leftSection,
        variant = "primary",
        size = "md",
        onClick,
        children,
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <button
        type={"button"}
        className={cx([
            "text-white",
            "flex items-center gap-1",
            "disabled:(bg-secondary-200 text-slate-600 cursor-not-allowed hover:(bg-secondary-200 shadow-none))",
            size ? cssSize[size] : undefined,
            variant ? cssVariant[variant] : undefined,
        ])}
        onClick={onClick}
        {...$props}
    >
        {leftSection}
        {children}
    </button>;
};
