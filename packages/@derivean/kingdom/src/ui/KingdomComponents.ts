import {withComponents}    from "@use-pico/extras";
import {KingdomRpc}        from "../rpc/KingdomRpc";
import {KingdomQueryStore} from "../store/KingdomQueryStore";

export const KingdomComponents = withComponents({
    rpc:        KingdomRpc,
    queryStore: KingdomQueryStore,
});
export const {
    Fetch:      KingdomFetch,
    Collection: KingdomCollection,
    Table:      KingdomTable,
} = KingdomComponents;
