import {type EventSchema} from "@derivean/event";
import {
    HeroIcon,
    InventoryIcon
}                         from "@derivean/ui";
import {
    Icon,
    Menu
}                         from "@use-pico/client";
import {type Infer}       from "@use-pico/extras";
import {t}                from "@use-pico/translator";
import {type FC}          from "react";

export namespace EventMenu {
    export interface Props extends Menu.PropsEx {
        event: Infer.Entity<EventSchema>;
    }
}

export const EventMenu: FC<EventMenu.Props> = (
    {
        event,
        ...props
    },
) => {
    const query = {id: event.id};

    return <Menu
        items={[
            {
                type:  "link",
                href:  "/manager/event/[id]",
                icon: <Icon icon={"i-tabler:search"}/>,
                query,
                label: t`Detail`,
            },
            event.type === "EventInventory" && {
                type:  "link",
                href:  `/manager/event/[id]/type/${event.type}`,
                icon:  <InventoryIcon/>,
                query,
                label: t`Event type [EventInventory]`,
            },
            event.type === "EventHero" && {
                type:  "link",
                href:  `/manager/event/[id]/type/${event.type}`,
                icon:  <HeroIcon/>,
                query,
                label: t`Event type [EventHero]`,
            },
        ]}
        {...props}
    />;
};
