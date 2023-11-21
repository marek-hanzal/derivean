import {withComponents}                            from "@use-pico/extras";
import {BuildingConstructionRequirementRpc}        from "../rpc/BuildingConstructionRequirementRpc";
import {BuildingConstructionRequirementQueryStore} from "../store/BuildingConstructionRequirementQueryStore";

export const {
    Fetch:      BuildingConstructionRequirementFetch,
    Collection: BuildingConstructionRequirementCollection,
} = withComponents({
    rpc:        BuildingConstructionRequirementRpc,
    queryStore: BuildingConstructionRequirementQueryStore,
});
