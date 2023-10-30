import {
    ResourceQueryStore,
    ResourceTable
}                      from "@derivean/resource";
import {ResourceIcon}  from "@derivean/ui";
import {StoreProvider} from "@use-pico2/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico2/ui";

export default function Index() {
    return <Page
        icon={<ResourceIcon/>}
        title={"manager.resource.list"}
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
                filter: {}
            }}
        >
            <ResourceTable/>
        </StoreProvider>
    </Page>;
}
