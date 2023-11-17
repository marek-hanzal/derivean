import {
    humanizer,
    type Options
}                 from "humanize-duration";
import {isString} from "./isString";

export const toHumanTime = (secs: number | string, options?: Options) => {
    return humanizer()(
        isString(secs) ? parseFloat(secs) * 1000 : secs * 1000,
        {
            units: [
                "mo",
                "d",
                "h",
                "m",
                "s",
                "ms",
            ],
            ...options
        }
    );
};
