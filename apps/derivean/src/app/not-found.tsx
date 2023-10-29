"use client";

import {
    defaultLocale,
    locales
} from "@/derivean/locales";
import {
    useLocaleOf,
    useLocaleRedirect
} from "@use-pico2/i18n";

export default function Custom404() {
    const localeOf = useLocaleOf({
        available: locales,
        fallback:  defaultLocale,
    });
    useLocaleRedirect({
        href: `./${localeOf}/public/not-found`,
    });
    return null;
};
