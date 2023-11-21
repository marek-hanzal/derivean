import {ProducerIcon} from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                     from "@use-pico/client";
import {t}            from "@use-pico/translator";

export default function List() {
    return <Page
        icon={<ProducerIcon/>}
        text={{
            header: t`Producer list`,
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
        {/*    store={ProducerQueryStore}*/}
        {/*    values={{}}*/}
        {/*>*/}
        {/*    <ProducerTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
