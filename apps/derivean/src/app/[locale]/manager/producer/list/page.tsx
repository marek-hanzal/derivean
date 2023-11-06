import {
    ProducerQueryStore,
    ProducerTable
}                      from "@derivean/producer";
import {ProducerIcon}  from "@derivean/ui";
import {tx}            from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<ProducerIcon/>}
        text={{
            title:  tx()`Producer list`,
            header: tx()`Producer list`,
        }}
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
