import {withComponents}     from "@use-pico/extras";
import {BuildingRpc}        from "../rpc/BuildingRpc";
import {BuildingQueryStore} from "../store/BuildingQueryStore";

export const BuildingComponents = withComponents({
    rpc:        BuildingRpc,
    queryStore: BuildingQueryStore,
});
export const {
    Fetch:      BuildingFetch,
    Collection: BuildingCollection,
    Table: BuildingTable,
} = BuildingComponents;
