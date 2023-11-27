import {cn}  from "@use-pico/utils";
import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs": [
        "h-2",
    ],
    "sm": [
        "h-4",
    ],
    "md": [
        "h-5",
    ],
    "lg": [
        "h-6",
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
        animated?: boolean;
    }
}

export const Progress: FC<Progress.Props> = (
    {
        value,
        size = "md",
        animated = false,
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "w-full bg-gray-200 rounded-md",
            ...cssSize[size],
        ])}
        {...$props}
    >
        <div
            className={cn([
                "h-full rounded-md bg-green-500",
                animated && "animate-pulse",
            ])}
            style={{width: "50%"}}
        />
    </div>;
};