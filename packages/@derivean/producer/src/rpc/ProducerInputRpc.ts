import {withDullRpc}             from "@use-pico/dull-stuff";
import {withDependenciesQuery}   from "../query/withDependenciesQuery";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";
import {ProducerInputSchema}     from "../schema/ProducerInputSchema";

export const ProducerInputRpc = withDullRpc({
    key:         ["derivean", "producer", "input"],
    schema:      ProducerInputSchema,
    invalidator: [
        withDependenciesQuery.key,
        withProductionTimeQuery.key,
    ],
});
export type ProducerInputRpc = typeof ProducerInputRpc;
