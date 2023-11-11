import {withDullRpc}    from "@use-pico/dull-stuff";
import {BuildingSchema} from "../schema/BuildingSchema";

export const BuildingRpc = withDullRpc({
    key:    ["derivean", "building"],
    schema: BuildingSchema,
});
