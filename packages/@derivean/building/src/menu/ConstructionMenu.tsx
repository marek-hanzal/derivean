import {ConstructionIcon} from "@derivean/ui";
import {IconHammer}       from "@tabler/icons-react";
import {t}                from "@use-pico/i18n";
import {Menu}             from "@use-pico/ui";
import {type FC}          from "react";

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
                label: t()`Kingdom current construction (label)`,
                icon:  <ConstructionIcon/>,
                href:  `/kingdom/[kingdomId]/building/construction/current`,
                query: {kingdomId},
            },
            {
                type:  "link",
                label: t()`Kingdom available buildings (label)`,
                icon:  <IconHammer/>,
                href:  `/kingdom/[kingdomId]/building/construction/available`,
                query: {kingdomId},
            },
        ]}
        {...props}
    />;
};
