import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs": "text-xs",
    "sm": "text-sm",
    "md": "text-md",
    "lg": "text-lg",
    "xl": "text-xl",
} as const;
type cssSize = typeof cssSize;

export namespace Icon {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        icon: string;
        size?: keyof cssSize;
    }
}

export const Icon: FC<Icon.Props> = (
    {
        icon,
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
            icon,
            cssSize[size],
        ])}
        {...$props}
    />;
};
