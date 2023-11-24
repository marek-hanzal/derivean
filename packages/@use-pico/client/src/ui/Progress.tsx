import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs": [],
    "sm": [],
    "md": [],
    "lg": [],
    "xl": [],
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
            ...cssSize[size],
        ])}
        {...$props}
    >
        progress
    </div>;
};
