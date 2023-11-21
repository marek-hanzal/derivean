import {
    type FC,
    type HTMLAttributes,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {type ILink}       from "../api/ILink";
import {isHrefProps}      from "../tools/isHrefProps";
import {tailwindify}      from "../tools/tailwindify";
import {ButtonLink}       from "./ButtonLink";

export namespace Menu {
    export interface Props extends HTMLAttributes<HTMLDivElement>, CommonProps {
        items: Items;
        active?: string[];
        linkProps?: Partial<ButtonLink.Props>;
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
        linkProps = {
            size: "sm",
        },
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
        {items.filter(Boolean).map(item => {
            if (isHrefProps(item)) {
                return <ButtonLink
                    key={item.href}
                    size={linkProps.size}
                    href={{
                        href:  item.href,
                        query: item.query,
                    }}
                    cn={[
                        "text-zinc-500",
                        "hover:text-zinc-700",
                        active?.includes(item.href) && "text-sky-500 hover:text-sky-600",
                        ...linkProps.cn ?? [],
                    ]}
                >
                    <div className={"flex flex-row gap-1 items-center"}>
                        {item.icon}
                        {item.label}
                    </div>
                </ButtonLink>;
            }
            return null;
        })}
    </div>;
};
