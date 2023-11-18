import {twMerge}          from "tailwind-merge";
import {type CommonProps} from "../api/CommonProps";
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

    return [twMerge(classes)];
};
