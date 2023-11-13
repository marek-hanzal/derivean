import {withDullUI}        from "@use-pico/dull-stuff";
import {KingdomQueryStore} from "../query/KingdomQueryStore";
import {KingdomRpc}        from "../rpc/KingdomRpc";

export const KingdomUI = withDullUI({
    rpc:        KingdomRpc,
    queryStore: KingdomQueryStore,
});
export type KingdomUI = typeof KingdomUI;
