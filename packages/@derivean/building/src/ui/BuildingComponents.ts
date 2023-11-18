import {withComponents}     from "@use-pico/extras";
import {BuildingQueryStore} from "../query/BuildingQueryStore";
import {BuildingRpc}        from "../rpc/BuildingRpc";

export const {
    Fetch:      BuildingFetch,
    Collection: BuildingCollection,
} = withComponents({
    rpc:        BuildingRpc,
    queryStore: BuildingQueryStore,
});
