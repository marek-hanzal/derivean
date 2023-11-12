import {withDullRpc}             from "@use-pico/dull-stuff";
import {withDependenciesQuery}   from "../query/withDependenciesQuery";
import {withGraph}               from "../query/withGraph";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";
import {ProducerInputSchema}     from "../schema/ProducerInputSchema";

export const ProducerInputRpc = withDullRpc({
    key:         ["derivean", "producer", "input"],
    schema:      ProducerInputSchema,
    invalidator: [
        withDependenciesQuery.key,
        withGraph.key,
        withProductionTimeQuery.key,
    ],
});
export type ProducerInputRpc = typeof ProducerInputRpc;
