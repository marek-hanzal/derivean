import {container}              from "@derivean/server";
import {
    withDefaultPipeline,
    withRichComponents
}                               from "@derivean/ui";
import {Providers}              from "@use-pico/client";
import {withTranslationService} from "@use-pico/server";
import {withInstance}           from "@use-pico/translator";
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
        translations: {
            ...parse(fs.readFileSync(`./src/translation/${locale}.yaml`, {encoding: "utf-8"})),
            ...await withTranslationService.use(container).translations(locale),
        },
        pipeline:     withDefaultPipeline(),
    });

    return <Providers
        locale={locale}
        translations={{
            translations,
            components: withRichComponents(),
        }}
    >
        {children}
    </Providers>;
}
