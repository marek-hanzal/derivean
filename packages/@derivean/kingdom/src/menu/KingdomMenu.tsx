import {IconHome2} from "@tabler/icons-react";
import {t}         from "@use-pico/i18n";
import {Menu}      from "@use-pico/ui";
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

    return <Menu
        items={[
            {
                type:  "link",
                label: t()`Overview`,
                icon:  <IconHome2/>,
                href:  `/game/kingdom/[kingdomId]`,
                query,
            }
        ]}
        {...props}
    />;
};
