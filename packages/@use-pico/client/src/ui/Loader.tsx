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
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        /**
         * Loader icon class name
         * https://icones.js.org/
         */
        loader?: string;
        size?: keyof cssSize;
    }
}

export const Loader: FC<Loader.Props> = (
    {
        loader = "i-svg-spinners:3-dots-scale",
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
            "opacity-100 text-sky-500",
            size,
            loader,
        ])}
        {...$props}
    />;
};
