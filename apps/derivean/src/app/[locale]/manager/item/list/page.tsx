import {ItemIcon} from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                 from "@use-pico/client";
import {t}        from "@use-pico/translator";

export default function List() {
    return <Page
        icon={<ItemIcon/>}
        text={{
            header: t()`Item list`,
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
        {/*    store={ItemQueryStore}*/}
        {/*    values={{}}*/}
        {/*>*/}
        {/*    <ItemTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
