import {withDullUI}              from "@use-pico/dull-stuff";
import {InventoryItemQueryStore} from "../query/InventoryItemQueryStore";
import {InventoryItemRpc}        from "../rpc/InventoryItemRpc";

export const InventoryItemUI = withDullUI({
    rpc:        InventoryItemRpc,
    queryStore: InventoryItemQueryStore,
});
export type InventoryItemUI = typeof InventoryItemUI;
