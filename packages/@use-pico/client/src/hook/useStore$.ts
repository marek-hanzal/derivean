import {useStore}    from "zustand";
import {type IStore} from "../api/IStore";
import {useContext$} from "./useContext$";

export function useStore$<
    TStore extends IStore<any>,
    TValue,
>(
    store: IStore.Store<TStore>,
    selector: (state: TStore["props"] & TStore["values"]) => TValue,
): TValue | null;

export function useStore$<
    TStore extends IStore<any>,
>(
    store: IStore.Store<TStore>,
): (TStore["props"] & TStore["values"]) | null;

export function useStore$<
    TStore extends IStore<any>,
>(
    store: IStore.Store<TStore>,
    selector?: <TValue>(state: TStore["props"] & TStore["values"]) => TValue,
) {
    const $store = useContext$(store.Context);
    if ($store) {
        // eslint-disable-next-line react-hooks/rules-of-hooks
        return selector ? useStore($store, selector) : useStore($store);
    }
    return null;
}
