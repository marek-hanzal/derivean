export interface CommonProps {
    pb?: CommonProps.Size;
    mb?: CommonProps.Size;
}

export namespace CommonProps {
    export type Size =
        | "xs"
        | "sm"
        | "md"
        | "lg"
        | "xl";

    export type Color =
        | "dimmed"
        | "primary";
}
