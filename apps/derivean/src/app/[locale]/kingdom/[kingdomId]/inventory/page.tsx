import {
    Inventory,
    InventoryItemQueryStore
}                              from "@derivean/inventory";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {InventoryIcon}         from "@derivean/ui";
import {t}                     from "@use-pico/i18n";
import {StoreProvider}         from "@use-pico/store";
import {Page}                  from "@use-pico/ui";

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
    >
        <StoreProvider
            store={InventoryItemQueryStore}
            values={{
                where: {
                    inventoryId: kingdom.inventoryId,
                },
            }}
        >
            <Inventory/>
        </StoreProvider>
    </Page>;
}
