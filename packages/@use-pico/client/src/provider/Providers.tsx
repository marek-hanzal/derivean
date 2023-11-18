"use client";

import {
    type ITranslations,
    withDefaultPipeline,
    withInstance,
    withRichComponents
}                            from "@use-pico/translator";
import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}                            from "react";
import {BlockStore}          from "../store/BlockStore";
import {DrawerStore}         from "../store/DrawerStore";
import {ModalStore}          from "../store/ModalStore";
import {QueryClientProvider} from "./QueryClientProvider";
import {RpcProvider}         from "./RpcProvider";

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
    withInstance({
        locale,
        translations: translations?.translations || {},
        pipeline:     withDefaultPipeline({
            rich: {
                component: {
                    components: translations?.components || withRichComponents(),
                }
            }
        }),
    });

    return <QueryClientProvider>
        <RpcProvider>
            <DrawerStore.Provider
                values={{
                    state: new Map(),
                }}
            >
                <ModalStore.Provider
                    values={{
                        state: new Map(),
                    }}
                >
                    <BlockStore.Provider
                        values={{
                            isBlock: false,
                        }}
                    >
                        {children}
                    </BlockStore.Provider>
                </ModalStore.Provider>
            </DrawerStore.Provider>
        </RpcProvider>
    </QueryClientProvider>;
};
