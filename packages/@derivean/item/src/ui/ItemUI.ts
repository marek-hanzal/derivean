import {withComponents} from "@use-pico/extras";
import {ItemQueryStore} from "../query/ItemQueryStore";
import {ItemRpc}        from "../rpc/ItemRpc";

export const {
    Fetch:      ItemFetch,
    Collection: ItemCollection,
} = withComponents({
    rpc:        ItemRpc,
    queryStore: ItemQueryStore,
});
