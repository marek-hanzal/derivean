"use client";

import {
    type ButtonHTMLAttributes,
    type FC,
    type ReactNode
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs":         "text-xs px-3 py-2 font-medium",
    "compact-xs": "text-xs px-3 py-1 font-medium",

    "sm":         "text-sm px-3 py-2 font-medium",
    "compact-sm": "text-sm px-3 py-1 font-medium",

    "md":         "text-md px-4 py-2 font-medium",
    "compact-md": "text-md px-3 py-1 font-medium",

    "lg":         "text-lg px-5 py-2 font-medium",
    "compact-lg": "text-lg px-4 py-1 font-medium",

    "xl":         "text-xl px-6 py-3 font-medium",
    "compact-xl": "text-xl px-5 py-2 font-medium",
} as const;
type cssSize = typeof cssSize;

const cssVariant = {
    "primary": `
        transition ease-in-out delay-50 duration-150
        bg-primary-600
        hover:shadow-slate-400 hover:shadow-md
    `,
    "subtle":  `
        text-slate-500
        transition ease-in-out delay-50 duration-150
        hover:bg-secondary-50
        hover:text-zinc-700
        hover:shadow-zinc-300 hover:shadow-md
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
        cn,
        $props
    } = css(props);

    return <button
        type={"button"}
        className={cn([
            `
                border-1
                rounded-md
                text-white
                
                flex gap-1
                
                disabled:bg-secondary-200
                disabled:text-slate-600
                disabled:hover:bg-secondary-200
                disabled:hover:shadow-none
                disabled:cursor-not-allowed
            `,
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
