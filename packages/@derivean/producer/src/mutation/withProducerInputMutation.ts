import {withRpcMutation}             from "@use-pico/rpc";
import {withProducerInputCountQuery} from "../query/input/withProducerInputCountQuery";
import {withProducerInputQuery}      from "../query/input/withProducerInputQuery";
import {ProducerInputMutationSchema} from "../schema/input/ProducerInputMutationSchema";
import {ProducerInputSchema}         from "../schema/input/ProducerInputSchema";

export const withProducerInputMutation = withRpcMutation({
    key:         ["derivean", "producer", "input", "mutation"],
    schema:      {
        request:  ProducerInputMutationSchema,
        response: ProducerInputSchema,
    },
    invalidator: async () => [
        withProducerInputCountQuery.key,
        withProducerInputQuery.key,
    ]
});
export type withProducerInputMutation = typeof withProducerInputMutation;
