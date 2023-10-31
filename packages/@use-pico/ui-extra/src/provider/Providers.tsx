"use client";

import {
    createTheme,
    MantineProvider,
    type MantineProviderProps
}                            from "@mantine/core";
import {ModalsProvider}      from "@mantine/modals";
import {Notifications}       from "@mantine/notifications";
import {
    DateTimeProvider,
    type IWithTranslationQuery,
    TranslationProvider,
    withQuery
}                            from "@use-pico/i18n";
import {QueryClientProvider} from "@use-pico/query";
import {RpcProvider}         from "@use-pico/rpc";
import {
    ActiveProvider,
    BlockProvider,
    DrawerStoreProvider,
    LoadingOverlay,
    ModalStoreProvider,
    RouterTransition
}                            from "@use-pico/ui";
import {
    type FC,
    type PropsWithChildren
}                            from "react";

export namespace Providers {
    export type Props = PropsWithChildren<{
        theme?: MantineProviderProps["theme"];
        /**
         * Set current locale
         */
        locale: string;
        /**
         * Translations used in the application
         */
        withTranslationQuery?: IWithTranslationQuery;
    }>;
}

export const Providers: FC<Providers.Props> = (
    {
        theme,
        locale,
        withTranslationQuery = withQuery({
            useCallback() {
                return async () => ({translations: {}});
            },
        }),
        children,
    }
) => {
    return <QueryClientProvider>
        <TranslationProvider
            withTranslationQuery={withTranslationQuery}
            locale={locale}
            loading={() => <LoadingOverlay visible/>}
        >
            <RpcProvider>
                <MantineProvider
                    theme={createTheme({
                        primaryColor: "blue",
                        primaryShade: 5,
                        ...theme
                    })}
                >
                    <RouterTransition/>
                    <Notifications position={"top-right"}/>
                    <ModalsProvider>
                        <DateTimeProvider
                            locale={locale}
                        >
                            <ActiveProvider>
                                <BlockProvider>
                                    <DrawerStoreProvider>
                                        <ModalStoreProvider>
                                            {children}
                                        </ModalStoreProvider>
                                    </DrawerStoreProvider>
                                </BlockProvider>
                            </ActiveProvider>
                        </DateTimeProvider>
                    </ModalsProvider>
                </MantineProvider>
            </RpcProvider>
        </TranslationProvider>
    </QueryClientProvider>;
};
