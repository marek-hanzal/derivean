import {twMerge}          from "tailwind-merge";
import {type CommonProps} from "../api/CommonProps";
import {twMargin}         from "./tailwindify/twMargin";
import {twPadding}        from "./tailwindify/twPadding";

export const tailwindify = (
    props: CommonProps
) => {
    const classes: string[] = [];

    props.px && classes.push(twPadding.x[props.px]);
    props.py && classes.push(twPadding.y[props.py]);
    props.pt && classes.push(twPadding.t[props.pt]);
    props.pb && classes.push(twPadding.b[props.pb]);
    props.p && classes.push(twPadding.p[props.p]);

    props.mx && classes.push(twMargin.x[props.mx]);
    props.my && classes.push(twMargin.y[props.my]);
    props.mt && classes.push(twMargin.t[props.mt]);
    props.mb && classes.push(twMargin.b[props.mb]);
    props.m && classes.push(twMargin.m[props.m]);

    return [twMerge(classes)];
};
