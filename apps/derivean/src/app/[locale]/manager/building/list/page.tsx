import {BuildingIcon} from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                     from "@use-pico/client";
import {t}            from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t()`Building list`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
            ]}
        />}
    >
        {/*<StoreProvider*/}
        {/*    store={BuildingQueryStore}*/}
        {/*    values={{*/}
        {/*        filter: {},*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <BuildingTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
