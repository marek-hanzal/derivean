import {
    type FC,
    HTMLAttributes,
    type ReactNode
}                    from "react";
import {CommonProps} from "../api/CommonProps";
import {type ILink}  from "../api/ILink";
import {isHrefProps} from "../tools/isHrefProps";
import {tailwindify} from "../tools/tailwindify";
import {ButtonLink}  from "./ButtonLink";

export namespace Menu {
    export interface Props extends HTMLAttributes<HTMLDivElement>, CommonProps {
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

export const Menu: FC<Menu.Props> = (
    {
        items,
        active,
        ...props
    }
) => {
    const {
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([
            "flex flex-row gap-2",
        ])}
        {...$props}
    >
        {items.filter(Boolean).map((item, index) => {
            if (isHrefProps(item)) {
                return <ButtonLink
                    size={"lg"}
                    href={{
                        href:  item.href,
                        query: item.query,
                    }}
                >
                    {item.icon}
                    {item.label}
                </ButtonLink>;
            }
            return null;
        })}
    </div>;
};
