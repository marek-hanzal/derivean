import {withComponents}                from "@use-pico/extras";
import {BuildingRequirementRpc}        from "../rpc/BuildingRequirementRpc";
import {BuildingRequirementQueryStore} from "../store/BuildingRequirementQueryStore";

export const {
    Fetch:      BuildingRequirementFetch,
    Collection: BuildingRequirementCollection,
} = withComponents({
    rpc:        BuildingRequirementRpc,
    queryStore: BuildingRequirementQueryStore,
});
