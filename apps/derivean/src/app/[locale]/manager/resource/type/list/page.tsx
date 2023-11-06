import {
    ResourceTypeQueryStore,
    ResourceTypeTable
}                      from "@derivean/resource";
import {tx}            from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<ListIcon/>}
        text={{
            title:  tx()`Resource type list`,
            header: tx()`Resource type list`,
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
            store={ResourceTypeQueryStore}
            values={{
                filter: {},
            }}
        >
            <ResourceTypeTable/>
        </StoreProvider>
    </Page>;
}
