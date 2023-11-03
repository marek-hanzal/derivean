import {
    BuildingQueryStore,
    BuildingTable
}                      from "@derivean/building";
import {BuildingIcon}  from "@derivean/ui";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<BuildingIcon/>}
        title={"manager.building.list"}
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
            store={BuildingQueryStore}
            values={{
                filter: {},
            }}
        >
            <BuildingTable/>
        </StoreProvider>
    </Page>;
}
