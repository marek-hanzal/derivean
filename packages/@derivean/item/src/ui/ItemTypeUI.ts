import {withDullUI}         from "@use-pico/dull-stuff";
import {ItemTypeQueryStore} from "../query/ItemTypeQueryStore";
import {ItemTypeRpc}        from "../rpc/ItemTypeRpc";

export const ItemTypeUI = withDullUI({
    rpc:        ItemTypeRpc,
    queryStore: ItemTypeQueryStore,
});
export type ItemTypeUI = typeof ItemTypeUI;
