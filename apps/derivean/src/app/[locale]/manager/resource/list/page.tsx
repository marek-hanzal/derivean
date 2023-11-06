import {
    ResourceQueryStore,
    ResourceTable
}                      from "@derivean/resource";
import {ResourceIcon}  from "@derivean/ui";
import {tx}            from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<ResourceIcon/>}
        text={{
            title:  tx()`Resource list`,
            header: tx()`Resource list`,
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
            store={ResourceQueryStore}
            values={{
                filter: {},
            }}
        >
            <ResourceTable/>
        </StoreProvider>
    </Page>;
}
