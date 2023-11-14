import {EventIcon}              from "@derivean/ui";
import {IconTimelineEventMinus} from "@tabler/icons-react";
import {t}                      from "@use-pico/i18n";
import {Menu}                   from "@use-pico/ui";
import {type FC}                from "react";

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
                label: t()`Current events (label)`,
                icon:  <EventIcon/>,
                href:  `/kingdom/[kingdomId]/events/current`,
                query: {kingdomId},
            },
            {
                type:  "link",
                label: t()`Past events (label)`,
                icon:  <IconTimelineEventMinus/>,
                href:  `/kingdom/[kingdomId]/events/past`,
                query: {kingdomId},
            },
        ]}
        {...props}
    />;
};
