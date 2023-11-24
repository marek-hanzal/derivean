import {withComponents}      from "@use-pico/extras";
import {InventoryRpc}        from "../rpc/InventoryRpc";
import {InventoryQueryStore} from "../store/InventoryQueryStore";

export const {
    Fetch:      InventoryFetch,
    Collection: InventoryCollection,
} = withComponents({
    rpc:        InventoryRpc,
    queryStore: InventoryQueryStore,
});
