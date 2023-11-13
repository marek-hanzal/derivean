import {
    BuildingIcon,
    EventIcon,
    ItemIcon,
    ProducerIcon
}                from "@derivean/ui";
import {
    IconGraph,
    IconLanguage
}                from "@tabler/icons-react";
import {t}       from "@use-pico/i18n";
import {
    ListIcon,
    MainMenu
}                from "@use-pico/ui";
import {type FC} from "react";

export namespace ManagerMenu {
    export interface Props {
    }
}

export const ManagerMenu: FC<ManagerMenu.Props> = () => {
    return <MainMenu
        items={[
            {
                type:  "group",
                label: t()`Item management`,
                icon:  <ItemIcon/>,
                items: [
                    {
                        type:  "link",
                        href:  "/manager/item/list",
                        label: t()`Item list`,
                        icon:  <ItemIcon/>,
                    },
                    {
                        type:  "link",
                        href:  "/manager/item/type/list",
                        label: t()`Item type list`,
                        icon:  <ListIcon/>,
                    },
                ],
            },
            {
                type:  "link",
                href:  "/manager/building/list",
                label: t()`Building list`,
                icon:  <BuildingIcon/>,
            },
            {
                type:  "group",
                label: t()`Producer management`,
                icon:  <ProducerIcon/>,
                items: [
                    {
                        type: "link",
                        href:  "/manager/producer/list",
                        label: t()`Producer list`,
                        icon:  <ListIcon/>,
                    },
                    {
                        type: "link",
                        href:  "/manager/producer/overview",
                        label: t()`Producer overview`,
                        icon:  <IconGraph/>,
                    },
                ],
            },
            {
                type:  "link",
                href:  "/manager/event/list",
                label: t()`Event list (label)`,
                icon:  <EventIcon/>,
            },
            {
                type:  "link",
                href:  "/manager/translation/list",
                label: t()`Translations`,
                icon:  <IconLanguage/>,
            },
        ]}
    />;
};
