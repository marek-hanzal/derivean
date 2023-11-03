import {
    ProducerQueryStore,
    ProducerTable
}                      from "@derivean/producer";
import {ProducerIcon}  from "@derivean/ui";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<ProducerIcon/>}
        title={"manager.producer.list"}
        postfix={<Breadcrumbs
            items={{
                "/manager": {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
            }}
        />}
    >
        <StoreProvider
            store={ProducerQueryStore}
            values={{
                filter: {},
            }}
        >
            <ProducerTable/>
        </StoreProvider>
    </Page>;
}
