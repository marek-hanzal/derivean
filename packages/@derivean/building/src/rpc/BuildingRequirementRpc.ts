import {withDullRpc}               from "@use-pico/dull-stuff";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export const BuildingRequirementRpc = withDullRpc({
    key:    ["derivean", "building", "requirement"],
    schema: BuildingRequirementSchema,
});
