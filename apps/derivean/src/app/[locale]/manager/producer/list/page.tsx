import {
    ProducerQueryStore,
    ProducerTable
}                      from "@derivean/producer";
import {ProducerIcon}  from "@derivean/ui";
import {t}             from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function List() {
    return <Page
        icon={<ProducerIcon/>}
        text={{
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
            values={{}}
        >
            <ProducerTable/>
        </StoreProvider>
    </Page>;
}
