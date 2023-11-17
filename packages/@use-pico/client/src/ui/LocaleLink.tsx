"use client";

import {isString}        from "@use-pico/utils";
import CoolLink          from "next/link";
import {
    type ComponentProps,
    type FC
}                        from "react";
import {type IHrefProps} from "../api/IHrefProps";
import {useLocaleRouter} from "../hook/useLocaleRouter";
import {linkTo}          from "../tools/linkTo";

export namespace LocaleLink {
    export interface Props extends Omit<ComponentProps<typeof CoolLink>, "href"> {
        href: IHrefProps | string;
        withLocale?: boolean;
    }
}

export const LocaleLink: FC<LocaleLink.Props> = (
    {
        href,
        withLocale = true,
        ...props
    }) => {
    const {locale} = useLocaleRouter();
    const $locale = withLocale ? locale : undefined;

    return <CoolLink
        href={linkTo(isString(href) ? {
            href: $locale ? `/${$locale}${href}` : href,
        } : {
            href:  $locale ? `/${$locale}${href.href}` : href.href,
            query: href.query,
        })}
        {...props}
    />;
};
