import {
    Inventory,
    InventoryItemQueryProvider
}                              from "@derivean/inventory";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {InventoryIcon}         from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {tv}                    from "@use-pico/translator";

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
            header: tv(kingdom)`Kingdom inventory`,
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
        <InventoryItemQueryProvider
            values={{
                where: {
                    inventoryId: kingdom.inventoryId,
                },
            }}
        >
            <Inventory/>
        </InventoryItemQueryProvider>
    </Page>;
}
