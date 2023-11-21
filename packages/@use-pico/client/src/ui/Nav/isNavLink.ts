import {isObject} from "@use-pico/utils";
import {type Nav} from "./Nav";

export const isNavLink = (item: any): item is Nav.Link => {
    if (!item || !isObject(item)) {
        return false;
    } else if ("type" in item && "href" in item) {
        return true;
    }
    return false;
};
