import {ResourceIcon} from "@derivean/ui";
import {MainMenu}     from "@use-pico2/ui";
import {type FC}      from "react";

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
            }
        }}
    />;
};
