import {LayoutShell}            from "@use-pico/ui-extra";
import {type PropsWithChildren} from "react";

// export async function generateStaticParams() {
//     return locales.map(locale => ({locale}));
// }

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
    const translations = (await import(`../../translation/${locale}.json`)).default;
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
