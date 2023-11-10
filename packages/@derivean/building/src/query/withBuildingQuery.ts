import {withRpcSourceQuery}     from "@use-pico/rpc";
import {BuildingSchema}         from "../schema/BuildingSchema";
import {withBuildingCountQuery} from "./withBuildingCountQuery";

export const withBuildingQuery = withRpcSourceQuery({
    key:            ["derivean", "building", "query"],
    schema:         {
        query:    BuildingSchema.query,
        response: BuildingSchema.entity
    },
    withCountQuery: withBuildingCountQuery,
});
