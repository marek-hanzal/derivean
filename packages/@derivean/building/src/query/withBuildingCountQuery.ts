import {CountSchema}    from "@use-pico/query";
import {withRpcQuery}   from "@use-pico/rpc";
import {BuildingSchema} from "../schema/BuildingSchema";

export const withBuildingCountQuery = withRpcQuery({
    key:    ["derivean", "building", "count"],
    schema: {
        request: BuildingSchema.query,
        response: CountSchema,
    },
});
