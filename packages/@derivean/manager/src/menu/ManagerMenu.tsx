import {
    BuildingIcon,
    ProducerIcon,
    ResourceIcon
}                  from "@derivean/ui";
import {IconGraph} from "@tabler/icons-react";
import {t}         from "@use-pico/i18n";
import {
    ListIcon,
    MainMenu
}                  from "@use-pico/ui";
import {type FC}   from "react";

export namespace ManagerMenu {
    export interface Props {
    }
}

export const ManagerMenu: FC<ManagerMenu.Props> = () => {
    return <MainMenu
        links={[
            {
                type:  "group",
                label: t()`Resource management`,
                icon:  <ResourceIcon/>,
                items: [
                    {
                        type:  "link",
                        href:  "/manager/resource/list",
                        label: t()`Resource list`,
                        icon:  <ResourceIcon/>,
                    },
                    {
                        type:  "link",
                        href:  "/manager/resource/type/list",
                        label: t()`Resource type list`,
                        icon:  <ListIcon/>,
                    },
                ],
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
                href:  "/manager/building/list",
                label: t()`Building list`,
                icon:  <BuildingIcon/>,
            },
        ]}
    />;
};
