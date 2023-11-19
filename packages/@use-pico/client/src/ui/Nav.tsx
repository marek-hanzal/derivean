import {
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {type IHrefProps}  from "../api/IHrefProps";

export namespace Nav {
    export interface Prop extends CommonProps {
        items: Item[];
        separator?: ReactNode;
        separatorMargin?: string;
    }

    export interface WithIcon {
        type: "label";
        icon: ReactNode;
        label?: ReactNode;
    }

    export interface WithLabel {
        type: "label";
        icon?: ReactNode;
        label: ReactNode;
    }

    export interface Link {
        type: "link";
        href: IHrefProps | string;
        label?: ReactNode;
        icon?: ReactNode;
    }

    export interface Custom {
        type: "custom";
        component: ReactNode;
    }

    export type Label =
        | WithIcon
        | WithLabel;

    export type Item =
        | Custom
        | Label
        | Link;
}

export const Nav: FC<Nav.Prop> = (
    {
        items,
    }
) => {
    return "Nav";
};
