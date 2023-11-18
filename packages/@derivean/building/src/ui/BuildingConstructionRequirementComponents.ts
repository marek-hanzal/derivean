import {withComponents}                            from "@use-pico/extras";
import {BuildingConstructionRequirementQueryStore} from "../query/BuildingConstructionRequirementQueryStore";
import {BuildingConstructionRequirementRpc}        from "../rpc/BuildingConstructionRequirementRpc";

export const {
    Fetch:      BuildingConstructionRequirementFetch,
    Collection: BuildingConstructionRequirementCollection,
} = withComponents({
    rpc:        BuildingConstructionRequirementRpc,
    queryStore: BuildingConstructionRequirementQueryStore,
});
