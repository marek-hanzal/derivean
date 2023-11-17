import {withItemTypeRepository} from "@derivean/item";
import {container}              from "@derivean/server";
import {ItemIcon}               from "@derivean/ui";
import {
    HomeIcon,
    ListIcon,
    Nav,
    Page
}                               from "@use-pico/client";
import {t}                      from "@use-pico/translator";

export namespace List {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function List({params: {id}}: List.Props) {
    const itemType = await withItemTypeRepository.use(container).getOrThrow(id);

    return <Page
        icon={<ItemIcon/>}
        text={{
            header: t({values: itemType})`Item type - item list`,
        }}
        postfix={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/item/type/list",
                    label: t()`Item type list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
    >
        {/*<StoreProvider*/}
        {/*    store={ItemQueryStore}*/}
        {/*    values={{*/}
        {/*        where: {*/}
        {/*            typeId: id,*/}
        {/*        },*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <ItemTable*/}
        {/*        resourceTypeId={itemType.id}*/}
        {/*    />*/}
        {/*</StoreProvider>*/}
    </Page>;
}
