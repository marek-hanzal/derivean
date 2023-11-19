"use client";

import {type IStore} from "../api/IStore";
import {createStore} from "./createStore";

export namespace ModalStore {
    export type StoreProps = IStore<{
        open(id: string): void;
        close(id: string): void;
        setOpen(id: string, isOpened: boolean): void;
        isOpen(id: string): boolean;
    }, {
        state: Map<string, boolean>;
    }>;
}

export const ModalStore = createStore<ModalStore.StoreProps>({
    name:    "ModalStore",
    factory: values => (set, get) => ({
        open:    id => {
            set({
                state: get().state.set(id, true),
            });
        },
        close:   id => {
            set({
                state: get().state.set(id, false),
            });
        },
        setOpen: (id, isOpened) => {
            set({
                state: get().state.set(id, isOpened),
            });
        },
        isOpen:  id => get().state.get(id) || false,
        ...values,
    }),
});