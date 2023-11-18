"use client";

import {type MutableRefObject} from "react";
import {type IBulkRef}         from "../api/IBulkRef";
import {type IStore}           from "../api/IStore";
import {createStore}           from "./createStore";

export namespace RpcStore {
    export type Store = IStore<
        IStore.Type,
        {
            bulkTimerRef: MutableRefObject<NodeJS.Timeout | undefined>;
            timeoutRef: MutableRefObject<NodeJS.Timeout | undefined>;
            bulkRef: MutableRefObject<Map<string, IBulkRef>>;
            url: string;
        }
    >;
}

export const RpcStore = createStore<RpcStore.Store>({
    name:    "RpcStore",
    factory: values => () => values,
});
