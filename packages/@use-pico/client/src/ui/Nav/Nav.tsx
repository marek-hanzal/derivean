import {generateId}       from "@use-pico/utils";
import {
    type FC,
    type HTMLAttributes,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../../api/CommonProps";
import {type IHrefProps}  from "../../api/IHrefProps";
import {tailwindify}      from "../../tools/tailwindify";
import {ButtonLink}       from "../ButtonLink";
import {isNavLabel}       from "./isNavLabel";
import {isNavLink}        from "./isNavLink";

export namespace Nav {
    export interface Prop extends HTMLAttributes<HTMLDivElement>, CommonProps {
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
        ...props
    }
) => {
    const {
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([
            "flex flex-row",
        ])}
        {...$props}
    >
        {items.map(item => {
            if (isNavLink(item)) {
                const {
                    type,
                    label,
                    ...$item
                } = item;

                return <ButtonLink
                    key={`breadcrumb-${generateId()}`}
                    {...$item}
                >
                    {label}
                </ButtonLink>;
            } else if (isNavLabel(item)) {
                const {
                    type,
                    ...$item
                } = item;

                return <div
                    key={`breadcrumb-${generateId()}`}
                    {...$item}
                >
                    {$item.icon}
                    {$item.label}
                </div>;
            }
            return null;
        })}
    </div>;
};
