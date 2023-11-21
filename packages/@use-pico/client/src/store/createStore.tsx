"use client";

import {
    createStore as coolCreateStore,
    type StateCreator,
    type StoreApi
}                      from "zustand";
import {type IStore}   from "../api/IStore";
import {useStore}      from "../hook/useStore";
import {useStore$}     from "../hook/useStore$";
import {StoreProvider} from "../provider/StoreProvider";
import {createContext} from "../tools/createContext";

export namespace createStore {
    export interface Props<
        TStore extends IStore<any>
    > {
        name?: string;
        factory: Factory<TStore>;
    }

    export type Factory<
        TStore extends IStore<any>
    > = (values: TStore["values"]) => StateCreator<TStore["props"] & TStore["values"]>;
}

/**
 * This is a helper to create (Zustand) store.
 */
export const createStore = <
    TStore extends IStore<any>
>(
    {
        name,
        factory,
    }: createStore.Props<TStore>
): IStore.Store<TStore> => {
    const Context: IStore.Store<TStore>["Context"] = createContext<StoreApi<TStore["props"] & TStore["values"]>>();
    const store: IStore.Store<TStore>["store"] = values => coolCreateStore<TStore["props"] & TStore["values"]>(
        ($set, $get, $store) => factory(values)($set, $get, $store)
    );

    return {
        /**
         * Store name.
         */
        name,
        /**
         * Store context.
         */
        Context,
        /**
         * Store factory.
         */
        store,
        useStore:     () => useStore({Context}),
        useStore$:    () => useStore$({Context}),
        useSelector:  selector => useStore({Context}, selector),
        useSelector$: selector => useStore$({Context}, selector),
        Provider:     props => <StoreProvider
            store={{
                Context,
                store
            }}
            {...props}
        />,
    };
};
