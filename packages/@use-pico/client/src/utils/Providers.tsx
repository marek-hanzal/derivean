import {type ITranslations}  from "@use-pico/translator";
import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}                            from "react";
import {QueryClientProvider} from "./QueryClientProvider";

export namespace Providers {
    export type Props = PropsWithChildren<{
        locale: string;
        translations?: {
            translations: ITranslations;
            components: Record<string, ReactNode>;
            // withQuery?: TranslationProvider.WithQuery;
        };
    }>;
}

/**
 * Wrapper over all the providers used in the application.
 */
export const Providers: FC<Providers.Props> = (
    {
        locale,
        translations,
        children,
    }
) => {
    return <QueryClientProvider>
        {children}
    </QueryClientProvider>;
};
