import {withComponents}                from "@use-pico/extras";
import {BuildingRequirementQueryStore} from "../query/BuildingRequirementQueryStore";
import {BuildingRequirementRpc}        from "../rpc/BuildingRequirementRpc";

export const {
    Fetch:      BuildingFetch,
    Collection: BuildingCollection,
} = withComponents({
    rpc:        BuildingRequirementRpc,
    queryStore: BuildingRequirementQueryStore,
});
