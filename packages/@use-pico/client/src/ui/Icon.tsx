import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

export namespace Icon {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
        icon: string;
    }
}

export const Icon: FC<Icon.Props> = (
    {
        icon,
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "text-2xl",
            icon,
        ])}
        {...$props}
    />;
};
