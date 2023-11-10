import {withRpcMutation}        from "@use-pico/rpc";
import {withBuildingCountQuery} from "../query/withBuildingCountQuery";
import {withBuildingQuery}      from "../query/withBuildingQuery";
import {BuildingSchema}         from "../schema/BuildingSchema";

export const withBuildingMutation = withRpcMutation({
    key:         ["derivean", "building", "mutation"],
    schema:      {
        request:  BuildingSchema.mutation,
        response: BuildingSchema.entity,
    },
    invalidator: async () => [
        withBuildingCountQuery.key,
        withBuildingQuery.key,
    ]
});
export type withBuildingMutation = typeof withBuildingMutation;
