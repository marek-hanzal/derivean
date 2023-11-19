import {twMargin}  from "../tools/tailwindify/twMargin";
import {twPadding} from "../tools/tailwindify/twPadding";

export interface CommonProps {
    /**
     * Any classes you want to pass to the component.
     *
     * They've a precedence over the `cn` class list.
     */
    className?: string | undefined;
    /**
     * Classes to be passed to the component.
     */
    cn?: (string | undefined)[];

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

    /**
     * TailwindCSS color.
     * https://tailwindcss.com/docs/customizing-colors
     */
    export type Color = string;
}
