import {type Prettify}    from "@use-pico/utils";
import {twMerge}          from "tailwind-merge";
import {type CommonProps} from "../api/CommonProps";
import {twMargin}         from "./tailwindify/twMargin";
import {twPadding}        from "./tailwindify/twPadding";

const cleanup: (keyof CommonProps)[] = [
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

export namespace tailwindify {
    export interface Result<
        TProps extends CommonProps,
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
        $props: Prettify<Omit<TProps, keyof CommonProps>>;

        /**
         * Prepare classes to be used directly in the component. Ensures TailwindCSS classes
         * are properly merged.
         */
        cn(classes?: (string | undefined)[]): string;
    }
}

export const tailwindify = <
    TProps extends CommonProps,
>(
    props: TProps
): tailwindify.Result<TProps> => {
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
        cn: input => twMerge(
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
