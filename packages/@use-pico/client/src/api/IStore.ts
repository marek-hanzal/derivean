import {
    type Context,
    FC
}                                 from "react";
import {type StoreApi}            from "zustand";
import {useStore as useCoolStore} from "../hook/useStore";
import {StoreProvider}            from "../provider/StoreProvider";

export interface IStore<
    TProps extends IStore.Type,
    TValues extends IStore.Type = IStore.Type,
> {
    props: TProps;
    values: TValues;
}

export namespace IStore {
    export type Type = Record<string, any>;

    export type Props<TStore extends IStore<any, any>> =
        TStore["props"]
        & TStore["values"];

    export type Api<
        TStore extends IStore<any>
    > = StoreApi<Props<TStore>>;

    export interface Store<
        TStore extends IStore<any>
    > {
        name?: string;
        Context: Context<Api<TStore> | null>;

        store(values: TStore["values"]): Api<TStore>;

        /**
         * Store provider shortcut
         */
        Provider: FC<Omit<StoreProvider.Props<TStore>, "store">>;

        /**
         * Use whole store
         */
        useStore(): ReturnType<typeof useCoolStore<TStore>>;

        /**
         * Use the store with a selector
         */
        useSelector<TValue>(selector: (state: TStore["props"] & TStore["values"]) => TValue): ReturnType<typeof useCoolStore<TStore, TValue>>;
    }
}
