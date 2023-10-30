"use client";

import {StoreProvider} from "@use-pico2/store";
import {
    type FC,
    type PropsWithChildren
}                      from "react";
import {ActiveStore}   from "./ActiveStore";

export namespace ActiveProvider {
    export type Props = PropsWithChildren<{
        defaultActive?: string[];
    }>;
}

export const ActiveProvider: FC<ActiveProvider.Props> = (
    {
        defaultActive,
        ...props
    }) => {
    return <StoreProvider
        store={ActiveStore}
        values={{
            active: defaultActive || [],
        }}
        {...props}
    />;
};
