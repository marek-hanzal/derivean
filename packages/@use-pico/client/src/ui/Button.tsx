"use client";

import {cn} from "@use-pico/utils";
import {
    ButtonHTMLAttributes,
    type FC,
    type PropsWithChildren,
    type ReactNode
}           from "react";

const twSize = {
    "xs": "text-xs px-3 py-2 font-medium",
    "sm": "text-sm px-3 py-2 font-medium",
    "md": "text-md px-4 py-2 font-medium",
    "lg": "text-lg px-5 py-2",
    "xl": "text-xl px-6 py-3",
};
type twSize = typeof twSize;

export namespace Button {
    export type Props = PropsWithChildren<
        {
            leftSection?: ReactNode;
            size?: Size;
        } & ButtonHTMLAttributes<HTMLButtonElement>
    >;

    export type Size = keyof twSize;
}

export const Button: FC<Button.Props> = (
    {
        size = "md",
        onClick,
        children,
    }
) => {
    return <button
        type={"button"}
        className={cn(`
            border-1
            rounded-md
            bg-gradient-to-r
            from-primary-500
            to-primary-600
            hover:bg-gradient-to-r
            hover:from-primary-600
            hover:to-primary-700
            text-white
        `, size ? twSize[size] : undefined)}
        onClick={onClick}
    >
        {children}
    </button>;
};
