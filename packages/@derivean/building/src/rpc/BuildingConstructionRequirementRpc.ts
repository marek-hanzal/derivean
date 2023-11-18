import {withRpc}                               from "@use-pico/extras";
import {BuildingConstructionRequirementSchema} from "../schema/BuildingConstructionRequirementSchema";

export const BuildingConstructionRequirementRpc = withRpc({
    key:    ["derivean", "building", "construction", "requirement"],
    schema: BuildingConstructionRequirementSchema,
});
