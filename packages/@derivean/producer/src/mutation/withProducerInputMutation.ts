import {withRpcMutation}             from "@use-pico/rpc";
import {withProducerInputCountQuery} from "../query/input/withProducerInputCountQuery";
import {withProducerInputQuery}      from "../query/input/withProducerInputQuery";
import {ProducerInputSchema}         from "../schema/ProducerInputSchema";

export const withProducerInputMutation = withRpcMutation({
    key:         ["derivean", "producer", "input", "mutation"],
    schema:      {
        request:  ProducerInputSchema.mutation,
        response: ProducerInputSchema.entity,
    },
    invalidator: async () => [
        withProducerInputCountQuery.key,
        withProducerInputQuery.key,
    ]
});
export type withProducerInputMutation = typeof withProducerInputMutation;
