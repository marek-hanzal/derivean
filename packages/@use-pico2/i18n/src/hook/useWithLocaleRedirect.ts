import {type IHrefProps} from "@use-pico2/navigation";
import {isString}        from "@use-pico2/utils";
import {useLocaleRouter} from "./useLocaleRouter";

export namespace useWithLocaleRedirect {
    export type Redirect = ReturnType<typeof useWithLocaleRedirect>;
}

export const useWithLocaleRedirect = () => {
    const {push} = useLocaleRouter();
    return (href?: IHrefProps | string | null) => href && push(isString(href) ? {
        href,
    } : {
        href:  href.href,
        query: href.query,
    });
};
