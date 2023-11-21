import {isObject} from "@use-pico/utils";
import {type Nav} from "./Nav";

export const isNavLabel = (item: any): item is Nav.Label => {
    if (!item || !isObject(item)) {
        return false;
    } else if ("type" in item && ("label" in item || "icon" in item)) {
        return true;
    }
    return false;
};
