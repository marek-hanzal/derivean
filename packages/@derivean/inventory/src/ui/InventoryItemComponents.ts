import {withComponents}          from "@use-pico/extras";
import {InventoryItemRpc}        from "../rpc/InventoryItemRpc";
import {InventoryItemQueryStore} from "../store/InventoryItemQueryStore";

export const {
    Fetch:      InventoryItemFetch,
    Collection: InventoryItemCollection,
    Table: InventoryItemTable,
} = withComponents({
    rpc:        InventoryItemRpc,
    queryStore: InventoryItemQueryStore,
});
