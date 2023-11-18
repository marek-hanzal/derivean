import {withComponents}          from "@use-pico/extras";
import {InventoryItemQueryStore} from "../query/InventoryItemQueryStore";
import {InventoryItemRpc}        from "../rpc/InventoryItemRpc";

export const {
    Fetch:      InventoryItemFetch,
    Collection: InventoryItemCollection,
} = withComponents({
    rpc:        InventoryItemRpc,
    queryStore: InventoryItemQueryStore,
});