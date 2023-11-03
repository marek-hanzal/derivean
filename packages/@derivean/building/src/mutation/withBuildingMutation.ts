import {withRpcMutation}        from "@use-pico/rpc";
import {withBuildingCountQuery} from "../query/withBuildingCountQuery";
import {withBuildingQuery}      from "../query/withBuildingQuery";
import {BuildingMutationSchema} from "../schema/BuildingMutationSchema";
import {BuildingSchema}         from "../schema/BuildingSchema";

export const withBuildingMutation = withRpcMutation({
    key:         ["derivean", "building", "mutation"],
    schema:      {
        request:  BuildingMutationSchema,
        response: BuildingSchema,
    },
    invalidator: async () => [
        withBuildingCountQuery.key,
        withBuildingQuery.key,
    ]
});
export type withBuildingMutation = typeof withBuildingMutation;
