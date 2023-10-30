"use client";

import {
    createStore,
    type IStore
} from "@use-pico2/store";

export namespace LinkLockStore {
    export type Props = IStore<{
        lock(lock?: boolean): void;
        unlock(): void;
    }, {
        isLock: boolean;
    }>;
}

export const LinkLockStore = createStore<LinkLockStore.Props>(values => (set) => ({
    lock:   (lock = true) => {
        set({
            isLock: lock,
        });
    },
    unlock: () => {
        set({
            isLock: false,
        });
    },
    ...values,
}));
