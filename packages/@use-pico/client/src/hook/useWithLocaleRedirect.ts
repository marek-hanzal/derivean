import {isString}        from "@use-pico/utils";
import {IHrefProps}      from "../api/IHrefProps";
import {useLocaleRouter} from "./useLocaleRouter";

export namespace useWithLocaleRedirect {
    export type Redirect = ReturnType<typeof useWithLocaleRedirect>;
}

/**
 * Return redirect function with locale.
 */
export const useWithLocaleRedirect = () => {
    const {push} = useLocaleRouter();
    return (href?: IHrefProps | string | null) => href && push(isString(href) ? {
        href,
    } : {
        href:  href.href,
        query: href.query,
    });
};
