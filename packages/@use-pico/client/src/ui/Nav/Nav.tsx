import {generateId}      from "@use-pico/utils";
import {
    type FC,
    type HTMLAttributes,
    type ReactNode
}                        from "react";
import {type IHrefProps} from "../../api/IHrefProps";
import {css}             from "../../tools/css";
import {ButtonLink}      from "../ButtonLink";
import {Text}            from "../Text";
import {isNavLabel}      from "./isNavLabel";
import {isNavLink}       from "./isNavLink";

export namespace Nav {
    export interface Prop extends HTMLAttributes<HTMLDivElement>, css.Style {
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
        separator = "|",
        separatorMargin = "mx-1",
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);
    const length = items.length;
    const $separator = <Text c={"text-zinc-400 font-bold"} className={separatorMargin}>{separator}</Text>;

    return <div
        className={cx([
            "flex flex-row items-center",
        ])}
        {...$props}
    >
        {items.map((item, index) => {
            if (isNavLink(item)) {
                const {
                    type,
                    label,
                    ...$item
                } = item;

                return <>
                    <ButtonLink
                        key={`breadcrumb-${generateId()}`}
                        {...$item}
                    >
                        {label}
                    </ButtonLink>
                    {index < length - 1 && $separator}
                </>;
            } else if (isNavLabel(item)) {
                const {
                    type,
                    ...$item
                } = item;

                return <>
                    <div
                        key={`breadcrumb-${generateId()}`}
                        {...$item}
                    >
                        {$item.icon}
                        {$item.label}
                    </div>
                    {index < length - 1 && $separator}
                </>;
            }
            return null;
        })}
    </div>;
};
