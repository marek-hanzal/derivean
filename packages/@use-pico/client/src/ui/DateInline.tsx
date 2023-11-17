"use client";

import {type IDateInput}            from "@use-pico/utils";
import {type DateTimeFormatOptions} from "luxon";
import {
    type FC,
    type HTMLProps
}                                   from "react";
import {useStore}                   from "../hook/useStore";
import {DateTimeStore}              from "../store/DateTimeStore";

export namespace DateInline {
    export interface Props extends Omit<HTMLProps<HTMLSpanElement>, "children"> {
        date?: IDateInput;
        fallback?: IDateInput;
        options?: DateTimeFormatOptions;
    }
}

export const DateInline: FC<DateInline.Props> = (
    {
        date,
        fallback,
        options,
        ...props
    }) => {
    const {toLocalDate} = useStore(DateTimeStore, ({toLocalDate}) => ({toLocalDate}));
    return <span {...props}>
        {toLocalDate(date, fallback, options)}
    </span>;
};
