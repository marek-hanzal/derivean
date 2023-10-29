import {type ILink} from "@use-pico2/navigation";

export interface IMenuLink<TPath extends string = string> extends ILink<TPath> {
    type: "link";
}
