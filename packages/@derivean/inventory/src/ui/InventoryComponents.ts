import {withComponents}      from "@use-pico/extras";
import {InventoryQueryStore} from "../query/InventoryQueryStore";
import {InventoryRpc}        from "../rpc/InventoryRpc";

export const {
    Fetch:      InventoryFetch,
    Collection: InventoryCollection,
} = withComponents({
    rpc:        InventoryRpc,
    queryStore: InventoryQueryStore,
});