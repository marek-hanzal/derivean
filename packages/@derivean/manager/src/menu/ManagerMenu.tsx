import {
    BuildingIcon,
    ResourceIcon
}                 from "@derivean/ui";
import {MainMenu} from "@use-pico/ui";
import {type FC}  from "react";

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
                type:  "link",
                href:  "/manager/resource/list",
                label: "link.resource.list",
                icon:  <ResourceIcon/>,
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
