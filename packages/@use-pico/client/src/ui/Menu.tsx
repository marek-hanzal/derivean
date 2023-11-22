import {
    type FC,
    type HTMLAttributes,
    type ReactNode
}                    from "react";
import {type ILink}  from "../api/ILink";
import {css}         from "../tools/css";
import {isHrefProps} from "../tools/isHrefProps";
import {ButtonLink}  from "./ButtonLink";

export namespace Menu {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
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
    } = css(props);

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
                        active?.includes(item.href) && "hover:cursor-default hover:shadow-none hover:bg-inherit",
                        !active?.includes(item.href) && "text-sky-500 hover:text-sky-600",
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
