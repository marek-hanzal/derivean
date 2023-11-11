import {
    BuildingQueryStore,
    BuildingTable
}                      from "@derivean/building";
import {BuildingIcon}  from "@derivean/ui";
import {tx}            from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<BuildingIcon/>}
        text={{
            title:  tx()`Building list`,
            header: tx()`Building list`,
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
            store={BuildingQueryStore}
            values={{
                filter: {},
            }}
        >
            <BuildingTable
                order={[
                    "dfsfg",
                ]}
            />
        </StoreProvider>
    </Page>;
}
