import {t}       from "@use-pico/i18n";
import {Menu}    from "@use-pico/ui";
import {type FC} from "react";

export namespace EventMenu {
    export interface Props extends Menu.PropsEx {
        eventId: string;
    }
}

export const EventMenu: FC<EventMenu.Props> = (
    {
        eventId,
        ...props
    },
) => {
    const query = {id: eventId};

    return <Menu
        items={[
            {
                type:  "link",
                href:  "/manager/event/[id]",
                query,
                label: t()`Detail`,
            },
            {
                type:  "link",
                href:  "/manager/event/[id]/type/EventItem",
                query,
                label: t()`Event type [EventItem]`,
            },
        ]}
        {...props}
    />;
};
