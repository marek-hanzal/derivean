"use client";

import {useEffect}   from "react";
import {localeOf}    from "../tools/localeOf";
import {useLocaleOf} from "./useLocaleOf";

export namespace useDetectLocale {
    export interface Props {
        locale: localeOf.Props;

        callback(props: localeOf.Props & {
            locale: string
        }): void;
    }
}

/**
 * Use this hook if you want to detect user's locale.
 *
 * Call the callback function with the detected locale.
 */
export const useDetectLocale = (
    {
        locale,
        callback,
    }: useDetectLocale.Props
) => {
    const $locale = useLocaleOf(locale);
    useEffect(() => {
        callback({
            ...locale,
            locale: $locale,
        });
    }, [$locale]);
    return $locale;
};
