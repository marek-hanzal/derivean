import {withRpcMutation}            from "@use-pico/rpc";
import {withResourceTypeCountQuery} from "../query/type/withResourceTypeCountQuery";
import {withResourceTypeQuery}      from "../query/type/withResourceTypeQuery";
import {withResourceCountQuery}     from "../query/withResourceCountQuery";
import {withResourceQuery}          from "../query/withResourceQuery";
import {ResourceTypeSchema}         from "../schema/ResourceTypeSchema";

export const withResourceTypeMutation = withRpcMutation({
    key:         ["derivean", "resource", "type", "mutation"],
    schema:      {
        request:  ResourceTypeSchema.mutation,
        response: ResourceTypeSchema.entity,
    },
    invalidator: async () => [
        withResourceTypeCountQuery.key,
        withResourceTypeQuery.key,
        withResourceCountQuery.key,
        withResourceQuery.key,
    ]
});
export type withResourceTypeMutation = typeof withResourceTypeMutation;
