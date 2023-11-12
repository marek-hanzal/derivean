import {
    ProducerOverview,
    ProducerQueryStore
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

export default function Overview() {
    return <Page
        icon={<ProducerIcon/>}
        text={{
            title:  tx()`Producer overview`,
            header: t()`Producer overview`,
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
            <ProducerOverview/>
        </StoreProvider>
    </Page>;
}
