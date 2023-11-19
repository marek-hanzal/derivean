import {
    type FC,
    type HTMLAttributes
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twVariant = {
    "dotted": "border-dotted",
} as const;
type twVariant = typeof twVariant;

export namespace Divider {
    export type Props =
        HTMLAttributes<HTMLHRElement>
        & CommonProps
        & {
            orientation?: Orientation;
            variant?: Variant;
        };

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
    const {cn, $props} = tailwindify(props);

    return <hr
        className={cn([
            "h-0.5 border bg-secondary-300 opacity-100",
            variant ? twVariant[variant] : undefined,
        ])}
        {...$props}
    />;
};
