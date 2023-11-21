import {
    BuildingIcon,
    ConstructionIcon
}                   from "@derivean/ui";
import {IconHammer} from "@tabler/icons-react";
import {Menu}       from "@use-pico/client";
import {t}          from "@use-pico/translator";
import {type FC}    from "react";

export namespace ConstructionMenu {
    export interface Props extends Menu.PropsEx {
        kingdomId: string;
    }
}

export const ConstructionMenu: FC<ConstructionMenu.Props> = (
    {
        kingdomId,
        ...props
    }
) => {
    return <Menu
        items={[
            {
                type:  "link",
                label: t`Kingdom building list (label)`,
                icon:  <BuildingIcon/>,
                href:  `/kingdom/[kingdomId]/building/list`,
                query: {kingdomId},
            },
            {
                type:  "link",
                label: t`Kingdom current construction (label)`,
                icon:  <ConstructionIcon/>,
                href:  `/kingdom/[kingdomId]/building/construction/current`,
                query: {kingdomId},
            },
            {
                type:  "link",
                label: t`Kingdom available buildings (label)`,
                icon:  <IconHammer/>,
                href:  `/kingdom/[kingdomId]/building/construction/available`,
                query: {kingdomId},
            },
        ]}
        {...props}
    />;
};
