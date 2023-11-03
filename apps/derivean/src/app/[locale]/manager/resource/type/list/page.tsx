import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
} from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<ListIcon/>}
        title={"manager.resource.type.list"}
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
        ResourceTypeTable

        {/*<StoreProvider*/}
        {/*    store={ResourceQueryStore}*/}
        {/*    values={{*/}
        {/*        filter: {},*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <ResourceTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
