import {
    defaultLocale,
    locales
}                       from "@/derivean/locales";
import {LocaleRedirect} from "@use-pico2/ui";
import {Providers}      from "@use-pico2/ui-extra";

export default function Index() {
    /**
     * Keep providers here as layout is too high for "empty" providers.
     */
    return <Providers
        locale={"en"}
    >
        <LocaleRedirect
            locale={{
                available: locales,
                fallback:  defaultLocale,
            }}
        />
    </Providers>;
}
