import {
    HomeIcon,
    ListIcon,
    Nav,
    Page
}          from "@use-pico/client";
import {t} from "@use-pico/translator";

export default function List() {
    return <Page
        icon={<ListIcon/>}
        text={{
            header: t()`Item type list`,
        }}
        postfix={<Nav
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
        {/*    store={ItemTypeQueryStore}*/}
        {/*    values={{*/}
        {/*        filter: {},*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <ItemTypeTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
