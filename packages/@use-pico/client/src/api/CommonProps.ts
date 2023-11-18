import {twMargin}  from "../tools/tailwindify/twMargin";
import {twPadding} from "../tools/tailwindify/twPadding";

export interface CommonProps {
    px?: keyof twPadding["x"];
    py?: keyof twPadding["y"];
    pt?: keyof twPadding["t"];
    pb?: keyof twPadding["b"];
    p?: keyof twPadding["p"];

    mx?: keyof twMargin["x"];
    my?: keyof twMargin["y"];
    mt?: keyof twMargin["t"];
    mb?: keyof twMargin["b"];
    m?: keyof twMargin["m"];
}

export namespace CommonProps {
    export type Variant =
        | "subtle";
}
