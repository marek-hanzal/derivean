import {isString}        from "@use-pico/utils";
import {type INavigate}  from "../api/INavigate";
import {linkTo}          from "../tools/linkTo";
import {useLocaleRouter} from "./useLocaleRouter";

/**
 * Generates a linkTo method variant connected to a locale.
 */
export const useLocaleLinkTo = (): INavigate<string> => {
    const {locale} = useLocaleRouter();
    return href => linkTo(isString(href) ? {
        href: `/${locale ?? ""}${href}`,
    } : {
        href:  `/${locale ?? ""}${href.href}`,
        query: href.query,
    });
};
