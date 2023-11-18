import {cn}               from "@use-pico/utils";
import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Divider {
    export type Props = PropsWithChildren<CommonProps & {
        orientation?: Orientation;
        variant?: Variant;
    }>;

    export type Variant =
        | "dotted";

    export type Orientation =
        | "horizontal"
        | "vertical";
}

export const Divider: FC<Divider.Props> = (
    {
        orientation = "horizontal",
        variant,
        ...props
    }
) => {
    return <hr
        className={cn(
            "h-0.5 border-t-0 bg-secondary-300 opacity-100",
            tailwindify(props),
        )}
    />;
};
