import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const cssSize = {
    "xs": "w-3 h-3",
    "sm": "w-3 h-3",
    "md": "w-3 h-3",
    "lg": "w-8 h-8",
    "xl": "w-16 h-16",
};
type cssSize = typeof cssSize;

export namespace Loader {
    export interface Props {
        /**
         * Loader icon class name
         */
        icon?: string;
        size?: keyof cssSize;
        /**
         * Style extensions of the inner elements
         */
        sx?: {
            root?: HTMLAttributes<HTMLDivElement>;
        };
    }
}

export const Loader: FC<Loader.Props> = (
    {
        icon = "i-svg-spinners-3-dots-scale",
        size = "md",
        sx,
    }
) => {
    const root = css(sx?.root || {});

    return <div
        className={root.cn([
            "self-center opacity-100 text-sky-500",
            size,
            icon,
        ])}
        {...root.$props}
    />;
};
