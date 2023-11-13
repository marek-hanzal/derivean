import {
    EventMenu,
    withEventInventoryRepository,
    withEventRepository
}                      from "@derivean/event";
import {
    InventoryItemQueryStore,
    InventoryItemTable
}                      from "@derivean/inventory";
import {container}     from "@derivean/server";
import {EventIcon}     from "@derivean/ui";
import {t}             from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                      from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const event = await withEventRepository.use(container).getOrThrow(id);
    const eventInventory = await withEventInventoryRepository.use(container).withQuery.fetchOrThrow({
        where: {
            eventId: event.id,
        }
    });

    return <Page
        icon={<EventIcon/>}
        text={{
            header: t({values: event})`Event type [EventInventory] (label)`,
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
            active={["/manager/event/[id]/type/EventInventory"]}
        />}
    >
        <StoreProvider
            store={InventoryItemQueryStore}
            values={{
                where: {
                    inventoryId: eventInventory.inventoryId,
                },
            }}
        >
            <InventoryItemTable
                hidden={["inventory"]}
                inventoryId={eventInventory.inventoryId}
            />
        </StoreProvider>
    </Page>;
}
