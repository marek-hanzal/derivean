import {
    ResourceQueryStore,
    ResourceTable
}                      from "@derivean/resource";
import {ResourceIcon}  from "@derivean/ui";
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
        icon={<ResourceIcon/>}
        text={{
            title:  tx()`Resource list`,
            header: t()`Resource list`,
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
            store={ResourceQueryStore}
            values={{
                filter: {},
            }}
        >
            <ResourceTable/>
        </StoreProvider>
    </Page>;
}
