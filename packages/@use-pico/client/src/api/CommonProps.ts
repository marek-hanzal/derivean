import {twPadding} from "../tools/tailwindify/twPadding";

export interface CommonProps {
    px?: keyof twPadding["x"];
    py?: keyof twPadding["y"];
    pt?: keyof twPadding["t"];
    pb?: keyof twPadding["b"];
    p?: keyof twPadding["p"];

    mx?: CommonProps.Size;
    my?: CommonProps.Size;
    mb?: CommonProps.Size;
    mt?: CommonProps.Size;
    m?: CommonProps.Size;
}

export namespace CommonProps {
    export type Variant =
        | "subtle";

    export type Size =
        | string
        | "xs"
        | "sm"
        | "md"
        | "lg"
        | "xl";

    export type Color =
        | "green"
        | "gray"
        | "yellow"
        | "orange"
        | "violet"
        | "grape"
        | "red"
        | "blue"
        /**
         * Specials
         */
        | "dimmed"
        | "primary";
}
