"use client";

import {Group}                 from "@mantine/core";
import {type IWithTranslation} from "@use-pico2/i18n";
import {isLink}                from "@use-pico2/navigation";
import {useStore}              from "@use-pico2/store";
import {cx}                    from "@use-pico2/utils";
import {usePathname}           from "next/navigation";
import {type FC}               from "react";
import {type IMenuItems}       from "../api/IMenuItems";
import {ActiveStore}           from "../store/ActiveStore";
import {isMenuGroup}           from "./isMenuGroup";
import classes                 from "./MainMenu.module.css";
import {MenuGroup}             from "./MenuGroup";
import {MenuLink}              from "./MenuLink";

export namespace MainMenu {
    export interface Props {
        withTranslation?: IWithTranslation;
        links: IMenuItems;
    }

    export type Classes = typeof classes;
}

export const MainMenu: FC<MainMenu.Props> = (
    {
        links,
        withTranslation,
    }) => {
    const {active: withActive} = useStore(ActiveStore, ({active}) => ({active}));
    const pathname = usePathname();
    return <Group
        className={classes.MenuGroup}
        gap={0}
    >
        {Object.entries(links).map(([id, item]) => {
            if (isLink(item)) {
                return <MenuLink
                    key={id}
                    id={id}
                    withTranslation={withTranslation}
                    className={cx(
                        classes.Link,
                        classes.LinkActive ? {
                            [classes.LinkActive]: pathname?.includes(item.href) || withActive.includes(item.href) || withActive.includes(id),
                        } : undefined
                    )}
                    {...item}
                />;
            } else if (isMenuGroup(item)) {
                return <MenuGroup
                    key={id}
                    id={id}
                    withTranslation={withTranslation}
                    className={classes.Link}
                    {...item}
                />;
            }
            return null;
        })}
    </Group>;
};
