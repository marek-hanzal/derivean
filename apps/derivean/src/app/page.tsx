import {
    defaultLocale,
    locales
}                       from "@/derivean/locales";
import {LocaleRedirect} from "@use-pico/ui";
import {Providers}      from "@use-pico/ui-extra";

export default function Index() {
    /**
     * Keep providers here as layout is too high for "empty" providers.
     */
    return <Providers
        locale={"en"}
        translations={{}}
    >
        <LocaleRedirect
            locale={{
                available: locales,
                fallback:  defaultLocale,
            }}
        />
    </Providers>;
}
