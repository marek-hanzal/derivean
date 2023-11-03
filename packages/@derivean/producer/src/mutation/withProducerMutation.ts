import {withRpcMutation}        from "@use-pico/rpc";
import {withProducerCountQuery} from "../query/withProducerCountQuery";
import {withProducerQuery}      from "../query/withProducerQuery";
import {ProducerMutationSchema} from "../schema/ProducerMutationSchema";
import {ProducerSchema}         from "../schema/ProducerSchema";

export const withProducerMutation = withRpcMutation({
    key:         ["derivean", "producer", "mutation"],
    schema:      {
        request:  ProducerMutationSchema,
        response: ProducerSchema,
    },
    invalidator: async () => [
        withProducerCountQuery.key,
        withProducerQuery.key,
    ]
});
export type withProducerMutation = typeof withProducerMutation;
