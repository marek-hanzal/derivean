"use client";

import {toHumanTime} from "@use-pico/utils";
import {type FC}     from "react";
import {useLocale}   from "../hook/useLocale";

export namespace HumanTime {
    export interface Props {
        seconds: number;
    }
}

export const HumanTime: FC<HumanTime.Props> = (
    {
        seconds,
    }
) => {
    const locale = useLocale();
    return toHumanTime(seconds, {
        language: locale,
    });
};
