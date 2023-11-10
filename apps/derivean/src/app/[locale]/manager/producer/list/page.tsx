import {
    ProducerQueryStore,
    ProducerTable
}                      from "@derivean/producer";
import {ProducerIcon}  from "@derivean/ui";
import {
    t,
    tx
}                      from "@use-pico/i18n";
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
            header: t()`Producer list`,
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
            store={ProducerQueryStore}
            values={{
                filter: {},
            }}
        >
            <ProducerTable/>
        </StoreProvider>
    </Page>;
}
