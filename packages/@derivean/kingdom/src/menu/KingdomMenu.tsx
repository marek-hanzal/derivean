import {
    BuildingIcon,
    EventIcon,
    InventoryIcon
}                         from "@derivean/ui";
import {IconBarrierBlock} from "@tabler/icons-react";
import {t}                from "@use-pico/i18n";
import {
    ListIcon,
    MainMenu,
    Menu
}                         from "@use-pico/ui";
import {type FC}          from "react";

export namespace KingdomMenu {
    export interface Props extends Menu.PropsEx {
        kingdomId: string;
    }
}

export const KingdomMenu: FC<KingdomMenu.Props> = (
    {
        kingdomId,
        ...props
    }
) => {
    const query = {kingdomId};

    return <MainMenu
        items={[
            {
                type:  "group",
                label: t()`Kingdom Buildings (label)`,
                icon:  <BuildingIcon/>,
                items: [
                    {
                        type:  "link",
                        label: t()`Kingdom Building List (label)`,
                        icon:  <ListIcon/>,
                        href:  `/kingdom/[kingdomId]/building/list`,
                        query,
                    },
                    {
                        type:  "link",
                        label: t()`Kingdom Building Construction (label)`,
                        icon:  <IconBarrierBlock/>,
                        href:  `/kingdom/[kingdomId]/building/construction`,
                        query,
                    },
                ],
            },
            {
                type:  "link",
                label: t()`Kingdom Inventory (label)`,
                icon:  <InventoryIcon/>,
                href:  `/kingdom/[kingdomId]/inventory`,
                query,
            },
            {
                type:  "link",
                label: t()`Events (label)`,
                icon:  <EventIcon/>,
                href: `/kingdom/[kingdomId]/events/current`,
                query,
            },
        ]}
        {...props}
    />;
};
