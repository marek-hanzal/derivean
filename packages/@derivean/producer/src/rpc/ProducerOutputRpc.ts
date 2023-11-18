import {withRpc}                 from "@use-pico/extras";
import {withDependenciesQuery}   from "../query/withDependenciesQuery";
import {withGraph}               from "../query/withGraph";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";
import {ProducerOutputSchema}    from "../schema/ProducerOutputSchema";

export const ProducerOutputRpc = withRpc({
    key:         ["derivean", "producer", "output"],
    schema:      ProducerOutputSchema,
    invalidator: [
        withDependenciesQuery.key,
        withGraph.key,
        withProductionTimeQuery.key,
    ],
});
export type ProducerOutputRpc = typeof ProducerOutputRpc;
