import {withDullUI}                    from "@use-pico/dull-stuff";
import {BuildingRequirementQueryStore} from "../query/BuildingRequirementQueryStore";
import {BuildingRequirementRpc}        from "../rpc/BuildingRequirementRpc";

export const BuildingRequirementUI = withDullUI({
    rpc:        BuildingRequirementRpc,
    queryStore: BuildingRequirementQueryStore,
});
export type BuildingRequirementUI = typeof BuildingRequirementUI;
