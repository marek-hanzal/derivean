import {withQuery}              from "@use-pico2/i18n";
import {LayoutShell}            from "@use-pico2/ui-extra";
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
            async callback() {
                return {
                    translations: (await import(`../../translation/${locale}.json`)).default,
                };
            }
        })}
    >
        {children}
    </LayoutShell>;
}
