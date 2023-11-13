import {
    ItemTypeQueryStore,
    ItemTypeTable
}                      from "@derivean/item";
import {
    t,
    tx
}                      from "@use-pico/i18n";
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
            title:  tx()`Item type list`,
            header: t()`Item type list`,
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
            store={ItemTypeQueryStore}
            values={{
                filter: {},
            }}
        >
            <ItemTypeTable/>
        </StoreProvider>
    </Page>;
}
