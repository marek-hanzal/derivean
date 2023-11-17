import {
    BuildingIcon,
    InventoryItemIcon,
    ItemIcon,
    PipelineIcon
}                from "@derivean/ui";
import {Menu}    from "@use-pico/client";
import {t}       from "@use-pico/translator";
import {type FC} from "react";

export namespace BuildingMenu {
    export interface Props extends Menu.PropsEx {
        buildingId: string;
    }
}

export const BuildingMenu: FC<BuildingMenu.Props> = (
    {
        buildingId,
        ...props
    }
) => {
    const query = {
        id: buildingId,
    };

    return <Menu
        items={[
            {
                type:  "link",
                href:  "/manager/building/[id]",
                label: t()`Building detail (label)`,
                icon:  <BuildingIcon/>,
                query,
            },
            {
                type:  "link",
                href:  "/manager/building/[id]/construction-requirement",
                label: t()`Building construction requirement (label)`,
                icon:  <InventoryItemIcon/>,
                query,
            },
            {
                type:  "link",
                href:  "/manager/building/[id]/requirement",
                label: t()`Building requirement (label)`,
                icon: <ItemIcon/>,
                query,
            },
            {
                type:  "link",
                href:  "/manager/building/[id]/pipeline",
                label: t()`Building pipeline (label)`,
                icon:  <PipelineIcon/>,
                query,
            },
        ]}
        {...props}
    />;
};
