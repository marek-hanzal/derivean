import {
    BuildingIcon,
    EventIcon,
    HeroIcon,
    InventoryIcon
}                from "@derivean/ui";
import {t}       from "@use-pico/i18n";
import {
    MainMenu,
    Menu
}                from "@use-pico/ui";
import {type FC} from "react";

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
                type:  "link",
                label: t()`Kingdom building list (label)`,
                icon:  <BuildingIcon/>,
                href:  `/kingdom/[kingdomId]/building/list`,
                query,
            },
            {
                type:  "link",
                label: t()`Kingdom hero list (label)`,
                icon:  <HeroIcon/>,
                href:  `/kingdom/[kingdomId]/hero/list`,
                query,
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
