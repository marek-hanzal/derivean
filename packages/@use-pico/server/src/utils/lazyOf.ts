import {
    type BindKey,
    get
} from "pumpit";

/**
 * Wrapper for lazy injected dependencies.
 */
export const lazyOf = (key: BindKey) => get(key, {lazy: true});
