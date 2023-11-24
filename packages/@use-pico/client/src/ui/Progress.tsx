import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs": [
        "h-8",
    ],
    "sm": [
        "h-8",
    ],
    "md": [
        "h-8",
    ],
    "lg": [
        "h-8",
    ],
    "xl": [
        "h-8",
    ],
} as const;
type cssSize = typeof cssSize;

export namespace Progress {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        value?: number;
        size?: keyof cssSize;
    }
}

export const Progress: FC<Progress.Props> = (
    {
        value,
        size = "md",
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "w-full bg-zinc-300",
            ...cssSize[size],
        ])}
        {...$props}
    >
        progress
    </div>;
};
