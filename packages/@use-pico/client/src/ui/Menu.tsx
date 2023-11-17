import {
    type FC,
    type ReactNode
}                   from "react";
import {type ILink} from "../api/ILink";

export namespace Menu {
    export interface Props {
        items: Items;
        active?: string[];
    }

    export type PropsEx = Omit<Props, "items">;

    export interface Link<TPath extends string = string> extends ILink<TPath> {
        type: "link";
        label?: ReactNode;
        icon?: ReactNode;
    }

    export interface Label {
        type: "label";
        label: ReactNode;
        icon?: ReactNode;
    }

    export interface Group {
        type: "group";
        label?: ReactNode;
        icon?: ReactNode;
        items: (Label | Link)[];
    }

    export type Items = (Link | Group | Label | undefined | null | false)[];
}

export const Menu: FC<Menu.Props> = () => {
    return "Menu";
};
