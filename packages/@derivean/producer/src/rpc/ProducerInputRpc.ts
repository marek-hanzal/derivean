import {withRpc}                 from "@use-pico/extras";
import {withDependenciesQuery}   from "../query/withDependenciesQuery";
import {withGraph}               from "../query/withGraph";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";
import {ProducerInputSchema}     from "../schema/ProducerInputSchema";

export const ProducerInputRpc = withRpc({
    key:         ["derivean", "producer", "input"],
    schema:      ProducerInputSchema,
    invalidator: [
        withDependenciesQuery.key,
        withGraph.key,
        withProductionTimeQuery.key,
    ],
});
export type ProducerInputRpc = typeof ProducerInputRpc;
