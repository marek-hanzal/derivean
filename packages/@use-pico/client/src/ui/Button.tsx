"use client";

import {
    type ButtonHTMLAttributes,
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twSize = {
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
type twSize = typeof twSize;

const twVariant = {
    "primary": `
        transition ease-in-out delay-50 duration-150
        bg-primary-600
        hover:shadow-slate-400 hover:shadow-md
    `,
    "subtle":  `
        text-slate-500
        transition ease-in-out delay-50 duration-150
        hover:bg-secondary-50
        hover:text-slate-700
        hover:shadow-slate-300 hover:shadow-md
    `,
} as const;
type twVariant = typeof twVariant;

export namespace Button {
    export type Props =
        ButtonHTMLAttributes<HTMLButtonElement>
        & CommonProps
        &
        {
            leftSection?: ReactNode;
            size?: Size;
            variant?: Variant;
        };

    export type Size = keyof twSize;
    export type Variant = keyof twVariant;
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
    } = tailwindify(props);

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
            size ? twSize[size] : undefined,
            variant ? twVariant[variant] : undefined,
        ])}
        onClick={onClick}
        {...$props}
    >
        {leftSection}
        {children}
    </button>;
};
