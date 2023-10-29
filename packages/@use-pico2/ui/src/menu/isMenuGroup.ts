import {isObject}        from "@use-pico2/utils";
import {type IMenuGroup} from "../api/IMenuGroup";

export const isMenuGroup = (item: any): item is IMenuGroup => {
    if (!item || !isObject(item)) {
        return false;
    } else if ("label" in item && "items" in item) {
        return true;
    }
    return false;
};
