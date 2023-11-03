import {withRpcMutation}          from "@use-pico/rpc";
import {withResourceCountQuery}   from "../query/withResourceCountQuery";
import {withResourceQuery}        from "../query/withResourceQuery";
import {ResourceMutationSchema}   from "../schema/ResourceMutationSchema";
import {ResourceRepositorySchema} from "../schema/ResourceRepositorySchema";

export const withResourceMutation = withRpcMutation({
    key:         ["derivean", "resource", "mutation"],
    schema:      {
        request:  ResourceMutationSchema,
        response: ResourceRepositorySchema,
    },
    invalidator: async () => [
        withResourceCountQuery.key,
        withResourceQuery.key,
    ]
});
export type withResourceMutation = typeof withResourceMutation;
