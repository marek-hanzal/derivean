import {
    withDefaultPipeline,
    withRichComponents
}                               from "@derivean/ui";
import {withInstance}           from "@use-pico/i18n";
import {LayoutShell}            from "@use-pico/ui-extra";
import fs                       from "node:fs";
import {type PropsWithChildren} from "react";
import {parse}                  from "yaml";

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
        translations: parse(fs.readFileSync(`./src/translation/${locale}.yaml`, {encoding: "utf-8"})),
        pipeline:     withDefaultPipeline(),
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
        translations={{
            translations,
            components: withRichComponents(),
        }}
    >
        {children}
    </LayoutShell>;
}
