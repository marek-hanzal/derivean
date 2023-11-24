import {
    type FC,
    type HTMLAttributes,
    type ReactNode
}            from "react";
import {css} from "../tools/css";

const twSize = {
    "xs": [
        "text-xs",
    ],
    "sm": [
        "text-sm",
    ],
    "md": [],
    "lg": [
        "text-xl",
    ],
    "xl": [
        "text-2xl",
    ],
};
type twSize = typeof twSize;

export namespace Rating {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        defaultValue?: number;
        count?: number;
        readOnly?: boolean;
        size?: Size;
        emptySymbol?: (value: number) => ReactNode;
        fullSymbol?: (value: number) => ReactNode;
    }

    export type Size = keyof twSize;
}

export const Rating: FC<Rating.Props> = (
    {
        defaultValue = 0,
        count = 5,
        readOnly = false,
        size = "md",
        emptySymbol,
        fullSymbol,
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "flex items-center justify-center gap-1",
            ...twSize[size],
        ])}
        {...$props}
    >
        {Array.from({length: count}).map((_, index) => index < defaultValue ? fullSymbol?.(index) : emptySymbol?.(index))}
    </div>;
};
