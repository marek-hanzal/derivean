import {withRpc}                   from "@use-pico/extras";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export const BuildingRequirementRpc = withRpc({
    key:    ["derivean", "building", "requirement"],
    schema: BuildingRequirementSchema,
});
