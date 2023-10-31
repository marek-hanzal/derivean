"use client";

import {withQuery}              from "@use-pico/i18n";
import {LayoutShell}            from "@use-pico/ui-extra";
import {type PropsWithChildren} from "react";

export namespace Layout {
    export type Props = PropsWithChildren<{
        params: {
            locale: string;
        },
    }>;
}

export default function Layout(
    {
        children,
        params: {locale}
    }: Layout.Props
) {
    return <LayoutShell
        theme={{
            /**
             * Default primary color (usually blue)
             */
            primaryColor: "blue",
            primaryShade: 5,
        }}
        locale={locale}
        withTranslationQuery={withQuery({
            useCallback() {
                return async () => ({
                    translations: (await import(`../../translation/${locale}.json`)).default,
                });
            }
        })}
    >
        {children}
    </LayoutShell>;
}
