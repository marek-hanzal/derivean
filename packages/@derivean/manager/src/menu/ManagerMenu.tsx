import {
    BuildingIcon,
    ResourceIcon
}                from "@derivean/ui";
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
        withTranslation={{
            namespace: "manager",
        }}
        links={{
            "/manager/resource": {
                type:  "group",
                label: "link.resource.label",
                icon:  <ResourceIcon/>,
                items: {
                    "/manager/resource/list":      {
                        type:  "link",
                        href:  "/manager/resource/list",
                        label: "link.resource.list",
                        icon:  <ResourceIcon/>,
                    },
                    "/manager/resource/type/list": {
                        type:  "link",
                        href:  "/manager/resource/type/list",
                        label: "link.resource.type.list",
                        icon:  <ListIcon/>,
                    },
                }
            },
            "/manager/building": {
                type:  "link",
                href:  "/manager/building/list",
                label: "link.building.list",
                icon:  <BuildingIcon/>,
            }
        }}
    />;
};
