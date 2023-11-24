import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

export namespace BoolInline {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        bool?: boolean | null;
        checkIcon?: string;
        unCheckIcon?: string;
        undefinedIcon?: string;
    }
}

export const BoolInline: FC<BoolInline.Props> = (
    {
        bool,
        checkIcon = "i-tabler:square-rounded-check",
        unCheckIcon = "i-tabler:square-rounded-x",
        undefinedIcon = "i-tabler:square-rounded",
        ...props
    }
) => {
    const {cx, $props} = css(props);

    return <div
        className={cx([
            bool !== undefined && bool !== null ? (bool ? `${checkIcon} text-green-500` : `${unCheckIcon} text-red-500`) : `${undefinedIcon} text-blue-500`,
            "text-2xl",
        ])}
        {...$props}
    />;
};
