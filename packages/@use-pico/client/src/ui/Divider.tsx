import {cn}               from "@use-pico/utils";
import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twVariant = {
    "dotted": "border-dotted",
} as const;
type twVariant = typeof twVariant;

export namespace Divider {
    export type Props = PropsWithChildren<CommonProps & {
        orientation?: Orientation;
        variant?: Variant;
    }>;

    export type Variant = keyof twVariant;

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
            "h-0.5 border bg-secondary-300 opacity-100",
            variant ? twVariant[variant] : undefined,
            tailwindify(props),
        )}
    />;
};
