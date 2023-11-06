import {
    BuildingIcon,
    ProducerIcon,
    ResourceIcon
}                from "@derivean/ui";
import {tx}      from "@use-pico/i18n";
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
        links={{
            "/manager/resource": {
                type:  "group",
                label: tx()`Resource management`,
                icon:  <ResourceIcon/>,
                items: {
                    "/manager/resource/list":      {
                        type:  "link",
                        href:  "/manager/resource/list",
                        label: tx()`Resource list`,
                        icon:  <ResourceIcon/>,
                    },
                    "/manager/resource/type/list": {
                        type:  "link",
                        href:  "/manager/resource/type/list",
                        label: tx()`Resource type list`,
                        icon:  <ListIcon/>,
                    },
                }
            },
            "/manager/producer": {
                type:  "link",
                href:  "/manager/producer/list",
                label: tx()`Producer list`,
                icon:  <ProducerIcon/>,
            },
            "/manager/building": {
                type:  "link",
                href:  "/manager/building/list",
                label: tx()`Building list`,
                icon:  <BuildingIcon/>,
            },
        }}
    />;
};
