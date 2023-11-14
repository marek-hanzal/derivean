import {
    defaultLocale,
    locales
}                       from "@/derivean/locales";
import {LocaleRedirect} from "@use-pico/ui";

export default function Index() {
    /**
     * Here is not loader, because LocaleRedirect is using `redirect` from
     * Next.js which trows an exception, so it would render nothing.
     */
    return <LocaleRedirect
        locale={{
            available: locales,
            fallback:  defaultLocale,
        }}
    />;
}
