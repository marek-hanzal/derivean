import {
    HeroIcon,
    InventoryIcon
}                       from "@derivean/ui";
import {IconSearch}     from "@tabler/icons-react";
import {withDullSchema} from "@use-pico/dull-stuff";
import {t}              from "@use-pico/i18n";
import {Menu}           from "@use-pico/ui";
import {type FC}        from "react";
import {EventSchema}    from "../schema/EventSchema";

export namespace EventMenu {
    export interface Props extends Menu.PropsEx {
        event: withDullSchema.Infer.Entity<EventSchema>;
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
                icon: <IconSearch/>,
                query,
                label: t()`Detail`,
            },
            event.type === "EventInventory" && {
                type:  "link",
                href: `/manager/event/[id]/type/${event.type}`,
                icon:  <InventoryIcon/>,
                query,
                label: t()`Event type [EventInventory]`,
            },
            event.type === "EventHero" && {
                type:  "link",
                href:  `/manager/event/[id]/type/${event.type}`,
                icon:  <HeroIcon/>,
                query,
                label: t()`Event type [EventHero]`,
            },
        ]}
        {...props}
    />;
};
