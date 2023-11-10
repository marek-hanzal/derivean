import {withRpcMutation}        from "@use-pico/rpc";
import {withResourceCountQuery} from "../query/withResourceCountQuery";
import {withResourceQuery}      from "../query/withResourceQuery";
import {ResourceSchema}         from "../schema/ResourceSchema";

export const withResourceMutation = withRpcMutation({
    key:         ["derivean", "resource", "mutation"],
    schema:      {
        request:  ResourceSchema.mutation,
        response: ResourceSchema.entity,
    },
    invalidator: async () => [
        withResourceCountQuery.key,
        withResourceQuery.key,
    ]
});
export type withResourceMutation = typeof withResourceMutation;
