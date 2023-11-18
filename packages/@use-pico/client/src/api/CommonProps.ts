export interface CommonProps {
    px?: CommonProps.Size;
    py?: CommonProps.Size;
    pt?: CommonProps.Size;
    pb?: CommonProps.Size;
    p?: CommonProps.Size;

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
        | number
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
