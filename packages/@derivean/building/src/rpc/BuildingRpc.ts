import {withRpc}        from "@use-pico/extras";
import {BuildingSchema} from "../schema/BuildingSchema";

export const BuildingRpc = withRpc({
    key:    ["derivean", "building"],
    schema: BuildingSchema,
});
