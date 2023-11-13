import {withDullUI}     from "@use-pico/dull-stuff";
import {ItemQueryStore} from "../query/ItemQueryStore";
import {ItemRpc}        from "../rpc/ItemRpc";

export const ItemUI = withDullUI({
    rpc:        ItemRpc,
    queryStore: ItemQueryStore,
});
export type ItemUI = typeof ItemUI;
