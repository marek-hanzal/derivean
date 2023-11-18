"use client";

import {cn}               from "@use-pico/utils";
import {
    type ButtonHTMLAttributes,
    type FC,
    type PropsWithChildren,
    type ReactNode
}                         from "react";
import {twMerge}          from "tailwind-merge";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twSize = {
    "xs": "text-xs px-3 py-2 font-medium",
    "sm": "text-sm px-3 py-2 font-medium",
    "md": "text-md px-4 py-2 font-medium",
    "lg": "text-lg px-5 py-2",
    "xl": "text-xl px-6 py-3",
} as const;
type twSize = typeof twSize;

const twVariant = {
    "primary": "bg-gradient-to-r from-primary-500 to-primary-600 hover:bg-gradient-to-r hover:from-primary-600 hover:to-primary-700",
    "subtle":  "bg-secondary-50 hover:bg-secondary-200 text-slate-500 hover:text-slate-700",
} as const;
type twVariant = typeof twVariant;

export namespace Button {
    export type Props = PropsWithChildren<
        CommonProps &
        {
            leftSection?: ReactNode;
            size?: Size;
            variant?: Variant;
        } & ButtonHTMLAttributes<HTMLButtonElement>
    >;

    export type Size = keyof twSize;
    export type Variant = keyof twVariant;
}

export const Button: FC<Button.Props> = (
    {
        variant = "primary",
        size = "md",
        onClick,
        children,
        className,
        ...props
    }
) => {
    return <button
        type={"button"}
        className={twMerge(
            cn(`
                border-1
                rounded-md
                text-white
            `,
                size ? twSize[size] : undefined,
                variant ? twVariant[variant] : undefined,
                tailwindify(props),
                className
            )
        )}
        onClick={onClick}
    >
        {children}
    </button>;
};
