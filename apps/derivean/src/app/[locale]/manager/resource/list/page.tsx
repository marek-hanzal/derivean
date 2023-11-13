import {
    ItemQueryStore,
    ItemTable
}                      from "@derivean/item";
import {ItemIcon}      from "@derivean/ui";
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
        icon={<ItemIcon/>}
        text={{
            title:  tx()`Item list`,
            header: t()`Item list`,
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
            store={ItemQueryStore}
            values={{}}
        >
            <ItemTable/>
        </StoreProvider>
    </Page>;
}
