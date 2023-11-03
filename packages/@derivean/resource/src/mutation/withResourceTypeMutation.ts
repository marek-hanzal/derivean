import {withRpcMutation}            from "@use-pico/rpc";
import {withResourceTypeCountQuery} from "../query/type/withResourceTypeCountQuery";
import {withResourceTypeQuery}      from "../query/type/withResourceTypeQuery";
import {withResourceCountQuery}     from "../query/withResourceCountQuery";
import {withResourceQuery}          from "../query/withResourceQuery";
import {ResourceTypeMutationSchema} from "../schema/type/ResourceTypeMutationSchema";
import {ResourceTypeSchema}         from "../schema/type/ResourceTypeSchema";

export const withResourceTypeMutation = withRpcMutation({
    key:         ["derivean", "resource", "type", "mutation"],
    schema:      {
        request:  ResourceTypeMutationSchema,
        response: ResourceTypeSchema,
    },
    invalidator: async () => [
        withResourceTypeCountQuery.key,
        withResourceTypeQuery.key,
        withResourceCountQuery.key,
        withResourceQuery.key,
    ]
});
export type withResourceTypeMutation = typeof withResourceTypeMutation;
