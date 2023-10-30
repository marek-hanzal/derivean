import {withMutation}               from "@use-pico2/query";
import {withResourceMutationAction} from "../action/withResourceMutationAction";
import {ResourceMutationSchema}     from "../schema/ResourceMutationSchema";
import {ResourceSchema}             from "../schema/ResourceSchema";

export const withResourceMutation = withMutation({
    key:     ["derivean", "resource", "mutation"],
    schema:  {
        request:  ResourceMutationSchema,
        response: ResourceSchema,
    },
    mutator: withResourceMutationAction,
});
export type withResourceMutation = typeof withResourceMutation;
