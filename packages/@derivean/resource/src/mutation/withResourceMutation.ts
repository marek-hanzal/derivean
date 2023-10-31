import {withMutation}               from "@use-pico/query";
import {withResourceMutationAction} from "../action/withResourceMutationAction";
import {withResourceCountQuery}     from "../query/withResourceCountQuery";
import {withResourceQuery}          from "../query/withResourceQuery";
import {ResourceMutationSchema}     from "../schema/ResourceMutationSchema";
import {ResourceSchema}             from "../schema/ResourceSchema";

export const withResourceMutation = withMutation({
    key:         ["derivean", "resource", "mutation"],
    schema:      {
        request:  ResourceMutationSchema,
        response: ResourceSchema,
    },
    mutator:     withResourceMutationAction,
    invalidator: async () => [
        withResourceQuery.key,
        withResourceCountQuery.key
    ],
});
export type withResourceMutation = typeof withResourceMutation;
