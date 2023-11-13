import {
    EventMenu,
    withEventRepository
}                  from "@derivean/event";
import {container} from "@derivean/server";
import {EventIcon} from "@derivean/ui";
import {t}         from "@use-pico/i18n";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                  from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const event = await withEventRepository.use(container).getOrThrow(id);

    return <Page
        icon={<EventIcon/>}
        text={{
            header: t({values: event})`EventItem settings (label)`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/event/list",
                    label: t()`Event list`,
                    icon:  <ListIcon/>,
                },
                {
                    type:  "link",
                    href:  {
                        href:  "/manager/event/[id]",
                        query: {id: event.id},
                    },
                    label: t({values: event})`Event detail`,
                    icon:  <EventIcon/>,
                },
            ]}
        />}
        append={<EventMenu
            eventId={event.id}
            active={["/manager/event/[id]/type/EventItem"]}
        />}
    >
        event item table
    </Page>;
}
