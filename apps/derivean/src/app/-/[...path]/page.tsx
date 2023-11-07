import {
    defaultLocale,
    locales
}                  from "@/derivean/locales";
import {
    LoadingOverlay,
    LocaleRedirect
}                  from "@use-pico/ui";
import {Providers} from "@use-pico/ui-extra";

export namespace Index {
    export interface Props {
        params: {
            path: string;
        };
    }
}

export default function Index({params: {path}}: Index.Props) {
    return <Providers
        locale={"en"}
    >
        <LocaleRedirect
            target={`/${path}`}
            locale={{
                available: locales,
                fallback:  defaultLocale,
            }}
        />
        <LoadingOverlay visible/>
    </Providers>;
}
