import {
    withDefaultPipeline,
    withInstance
}                               from "@use-pico/i18n";
import {LayoutShell}            from "@use-pico/ui-extra";
import {type PropsWithChildren} from "react";

export namespace Layout {
    export type Props = PropsWithChildren<{
        params: {
            locale: string;
        },
    }>;
}

export default async function Layout(
    {
        children,
        params: {locale}
    }: Layout.Props
) {
    const {translations} = withInstance({
        locale,
        translations: (await import(`../../translation/${locale}.json`)).default,
        pipeline: withDefaultPipeline(),
    });

    return <LayoutShell
        theme={{
            /**
             * Default primary color (usually blue)
             */
            primaryColor: "blue",
            primaryShade: 5,
        }}
        locale={locale}
        translations={translations}
    >
        {children}
    </LayoutShell>;
}
