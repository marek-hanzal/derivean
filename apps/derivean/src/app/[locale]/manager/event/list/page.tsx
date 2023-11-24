import {
    EventQueryProvider,
    EventTable
}                  from "@derivean/event";
import {EventIcon} from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                  from "@use-pico/client";
import {t}         from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<EventIcon/>}
        text={{
            header: t`Event list`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
            ]}
        />}
    >
        <EventQueryProvider
            values={{
                filter: {},
            }}
        >
            <EventTable/>
        </EventQueryProvider>
    </Page>;
}
