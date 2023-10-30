"use client";

import {
    createStore,
    type IStore
} from "@use-pico2/store";

export namespace LoopsStore {
    export type Props = IStore<{
        current: number;

        isRunning(): boolean;

        inc(): void;

        dec(): void;
    }>
}

export const LoopsStore = createStore<LoopsStore.Props>(() => (set, get) => ({
    current: 0,
    isRunning() {
        return get().current > 0;
    },
    inc() {
        set(({current}) => ({current: current + 1}));
    },
    dec() {
        set(({current}) => ({current: current - 1}));
    },
}));
