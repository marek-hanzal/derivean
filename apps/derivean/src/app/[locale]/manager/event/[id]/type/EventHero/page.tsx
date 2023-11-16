import {
    EventHeroUpsertForm,
    EventMenu,
    withEventHeroRepository,
    withEventRepository
}                  from "@derivean/event";
import {container} from "@derivean/server";
import {EventIcon} from "@derivean/ui";
import {t}         from "@use-pico/i18n";
import {
    Breadcrumbs,
    Container,
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
    const eventHero = await withEventHeroRepository.use(container).withQuery.fetchOrThrow({
        where: {
            eventId: event.id,
        }
    });

    return <Page
        icon={<EventIcon/>}
        text={{
            header: t({values: event})`Event type [EventHero] (label)`,
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
            ]}
        />}
        append={<EventMenu
            event={event}
            active={["/manager/event/[id]/type/EventHero"]}
        />}
    >
        <Container>
            <EventHeroUpsertForm
                entity={eventHero}
                refresh
            />
        </Container>
    </Page>;
}
