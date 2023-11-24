import {type Prettify} from "@use-pico/utils";
import {twMerge}       from "tailwind-merge";
import {twMargin}      from "./tailwindify/twMargin";
import {twPadding}     from "./tailwindify/twPadding";

const cleanup: (keyof css.Style)[] = [
    "cn",
    "className",
    "px",
    "py",
    "pt",
    "pb",
    "p",
    "mx",
    "my",
    "mt",
    "mb",
    "m",
];

export namespace css {
    export interface Result<
        TProps extends Style,
    > {
        /**
         * Pure (unmodified) list of tailwind classes.
         */
        tw: string[];
        /**
         * Cleaned up props, so you can pass them to the next component (for example div, ...).
         *
         * Dollar name, because you would usually have "props" on input, so this will be out
         * of collision, when you destructure it.
         */
        $props: Prettify<Omit<TProps, keyof Style>>;

        /**
         * Prepare classes to be used directly in the component. Ensures TailwindCSS classes
         * are properly merged.
         */
        cx(classes?: (string | undefined)[]): string;
    }

    export interface Style {
        /**
         * Any classes you want to pass to the component.
         *
         * They've a precedence over the `cn` class list.
         */
        className?: string | undefined;
        /**
         * Classes to be passed to the component.
         */
        cn?: (string | undefined | false)[];

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

    export type Variant =
        | "subtle";

    /**
     * If some component supports size, this is a default list of sizes.
     */
    export type Size =
        | "xs"
        | "sm"
        | "md"
        | "lg"
        | "xl"
        | "xl2"

    /**
     * TailwindCSS/UnoCSS color string.
     * https://tailwindcss.com/docs/customizing-colors
     */
    export type Color = string;
}

export const css = <
    TProps extends css.Style,
>(
    props: TProps
): css.Result<TProps> => {
    const classes: string[] = [];

    const $props = {...props};

    $props.px && classes.push(twPadding.x[$props.px]);
    $props.py && classes.push(twPadding.y[$props.py]);
    $props.pt && classes.push(twPadding.t[$props.pt]);
    $props.pb && classes.push(twPadding.b[$props.pb]);
    $props.p && classes.push(twPadding.p[$props.p]);

    $props.mx && classes.push(twMargin.x[$props.mx]);
    $props.my && classes.push(twMargin.y[$props.my]);
    $props.mt && classes.push(twMargin.t[$props.mt]);
    $props.mb && classes.push(twMargin.b[$props.mb]);
    $props.m && classes.push(twMargin.m[$props.m]);

    for (const key of cleanup) {
        $props[key] = undefined;
        delete $props[key];
    }

    return {
        tw: classes,
        $props,
        cx: input => twMerge(
            ...classes,
            ...input?.filter(Boolean) || [],
            /**
             * Things passed through props wins.
             */
            props.className,
            ...props.cn?.filter(Boolean) || [],
        ),
    };
};
