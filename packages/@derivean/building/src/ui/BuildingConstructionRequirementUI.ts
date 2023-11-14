import {withDullUI}                                from "@use-pico/dull-stuff";
import {BuildingConstructionRequirementQueryStore} from "../query/BuildingConstructionRequirementQueryStore";
import {BuildingConstructionRequirementRpc}        from "../rpc/BuildingConstructionRequirementRpc";

export const BuildingConstructionRequirementUI = withDullUI({
    rpc:        BuildingConstructionRequirementRpc,
    queryStore: BuildingConstructionRequirementQueryStore,
});
export type BuildingConstructionRequirementUI = typeof BuildingConstructionRequirementUI;
