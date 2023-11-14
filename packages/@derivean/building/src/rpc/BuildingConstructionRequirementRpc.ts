import {withDullRpc}                           from "@use-pico/dull-stuff";
import {BuildingConstructionRequirementSchema} from "../schema/BuildingConstructionRequirementSchema";

export const BuildingConstructionRequirementRpc = withDullRpc({
    key:    ["derivean", "building", "construction", "requirement"],
    schema: BuildingConstructionRequirementSchema,
});
