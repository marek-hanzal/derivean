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
        cn,
        $props
    } = css(props);

    return <div
        className={cn([
            icon,
        ])}
        {...$props}
    />;
};
