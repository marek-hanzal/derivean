import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {InventoryIcon}         from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Index({params: {kingdomId}}: Index.Props) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <Page
        icon={<InventoryIcon/>}
        text={{
            header: t({values: kingdom})`Kingdom inventory`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href:  "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
            ]}
        />}
    >
        {/*<StoreProvider*/}
        {/*    store={InventoryItemQueryStore}*/}
        {/*    values={{*/}
        {/*        where: {*/}
        {/*            inventoryId: kingdom.inventoryId,*/}
        {/*        },*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <Inventory/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
