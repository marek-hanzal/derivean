import {get}          from "pumpit";
import {type BindKey} from "pumpit/src/types";

export const lazyOf = (key: BindKey) => get(key, {lazy: true});
