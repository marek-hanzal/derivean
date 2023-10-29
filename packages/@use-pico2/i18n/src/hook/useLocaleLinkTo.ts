import {
    type IWithLinkTo,
    linkTo
}                        from "@use-pico2/navigation";
import {isString}        from "@use-pico2/utils";
import {useLocaleRouter} from "./useLocaleRouter";

export const useLocaleLinkTo = (): IWithLinkTo => {
    const {locale} = useLocaleRouter();
    return href => linkTo(isString(href) ? {
        href: `/${locale ?? ""}${href}`,
    } : {
        href:  `/${locale ?? ""}${href.href}`,
        query: href.query,
    });
};
