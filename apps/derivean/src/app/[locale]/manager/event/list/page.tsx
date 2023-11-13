import {
    EventQueryStore,
    EventTable
}                      from "@derivean/event";
import {EventIcon}     from "@derivean/ui";
import {tx}            from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<EventIcon/>}
        text={{
            header: tx()`Event list`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
            ]}
        />}
    >
        <StoreProvider
            store={EventQueryStore}
            values={{
                filter: {},
            }}
        >
            <EventTable/>
        </StoreProvider>
    </Page>;
}
