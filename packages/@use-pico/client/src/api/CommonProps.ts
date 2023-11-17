export interface CommonProps {
    px?: CommonProps.Size;
    pt?: CommonProps.Size;
    pb?: CommonProps.Size;
    mb?: CommonProps.Size;
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
        | "dimmed"
        | "green"
        | "red"
        | "blue"
        | "primary";
}
