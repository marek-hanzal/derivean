import {EventIcon} from "@derivean/ui";
import {
    Icon,
    Menu
}                  from "@use-pico/client";
import {t}         from "@use-pico/translator";
import {type FC}   from "react";

export namespace EventMenu {
    export interface Props extends Menu.PropsEx {
        kingdomId: string;
    }
}

export const EventMenu: FC<EventMenu.Props> = (
    {
        kingdomId,
        ...props
    }
) => {
    return <Menu
        items={[
            {
                type:  "link",
                label: t`Current events (label)`,
                icon:  <EventIcon/>,
                href:  `/kingdom/[kingdomId]/events/current`,
                query: {kingdomId},
            },
            {
                type:  "link",
                label: t`Past events (label)`,
                icon: <Icon icon={"i-tabler-timeline-event-minus"}/>,
                href:  `/kingdom/[kingdomId]/events/past`,
                query: {kingdomId},
            },
        ]}
        {...props}
    />;
};
