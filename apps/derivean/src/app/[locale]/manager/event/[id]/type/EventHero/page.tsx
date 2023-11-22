import {
    withEventHeroRepository,
    withEventRepository
}                  from "@derivean/event";
import {EventMenu} from "@derivean/manager";
import {container} from "@derivean/server";
import {EventIcon} from "@derivean/ui";
import {
    HomeIcon,
    ListIcon,
    Nav,
    Page
}                  from "@use-pico/client";
import {
    t,
    tv
}                  from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const event = await withEventRepository.use(container).getOrThrow(id);
    const eventHero = await withEventHeroRepository.use(container).withQuery.fetchOrThrow({
        where: {
            eventId: event.id,
        }
    });

    return <Page
        icon={<EventIcon/>}
        text={{
            header: tv(event)`Event type [EventHero] (label)`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/event/list",
                    label: t`Event list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        menu={<EventMenu
            event={event}
            active={["/manager/event/[id]/type/EventHero"]}
        />}
    >
        {/*<EventHeroUpsertForm*/}
        {/*    entity={eventHero}*/}
        {/*    refresh*/}
        {/*/>*/}
    </Page>;
}
