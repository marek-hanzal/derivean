import {
    DateTime,
    type DateTimeFormatOptions
}                        from "luxon";
import {type IDateInput} from "../api/IDateInput";
import {fromUtc}         from "./fromUtc";
import {isDateTime}      from "./isDateTime";
import {isString}        from "./isString";

export const iso2locale = (date?: IDateInput, fallback?: IDateInput, opts?: DateTimeFormatOptions): string | undefined => {
    if (!date) {
        date = fallback;
    }
    if (!date) {
        return undefined;
    }
    if (isString(date)) {
        return fromUtc(date).toLocaleString(opts);
    } else if (isDateTime(date)) {
        return date.toLocaleString(opts);
    }
    return DateTime.fromJSDate(date).toLocaleString(opts);
};
