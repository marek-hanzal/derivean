import {withRpcSourceQuery}     from "@use-pico/rpc";
import {BuildingQuerySchema}    from "../schema/BuildingQuerySchema";
import {BuildingSchema}         from "../schema/BuildingSchema";
import {withBuildingCountQuery} from "./withBuildingCountQuery";

export const withBuildingQuery = withRpcSourceQuery({
    key:            ["derivean", "building", "query"],
    schema:         {
        query:    BuildingQuerySchema,
        response: BuildingSchema,
    },
    withCountQuery: withBuildingCountQuery,
});
