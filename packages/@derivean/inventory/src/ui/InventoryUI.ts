import {withDullUI}          from "@use-pico/dull-stuff";
import {InventoryQueryStore} from "../query/InventoryQueryStore";
import {InventoryRpc}        from "../rpc/InventoryRpc";

export const InventoryUI = withDullUI({
    rpc:        InventoryRpc,
    queryStore: InventoryQueryStore,
});
export type InventoryUI = typeof InventoryUI;
