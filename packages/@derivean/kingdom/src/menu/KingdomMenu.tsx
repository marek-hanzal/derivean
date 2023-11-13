import {
    EventIcon,
    InventoryIcon
}                  from "@derivean/ui";
import {IconHome2} from "@tabler/icons-react";
import {t}         from "@use-pico/i18n";
import {
    MainMenu,
    Menu
}                  from "@use-pico/ui";
import {type FC}   from "react";

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
                label: t()`Kingdom overview (label)`,
                icon:  <IconHome2/>,
                href:  `/kingdom/[kingdomId]`,
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
                href:  `/kingdom/[kingdomId]/events`,
                query,
            },
        ]}
        {...props}
    />;
};
