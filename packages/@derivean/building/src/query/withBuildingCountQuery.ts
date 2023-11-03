import {CountSchema}         from "@use-pico/query";
import {withRpcQuery}        from "@use-pico/rpc";
import {BuildingQuerySchema} from "../schema/BuildingQuerySchema";

export const withBuildingCountQuery = withRpcQuery({
    key:    ["derivean", "building", "count"],
    schema: {
        request:  BuildingQuerySchema,
        response: CountSchema,
    },
});
