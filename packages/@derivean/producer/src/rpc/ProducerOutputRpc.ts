import {withDullRpc}             from "@use-pico/dull-stuff";
import {withDependenciesQuery}   from "../query/withDependenciesQuery";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";
import {ProducerOutputSchema}    from "../schema/ProducerOutputSchema";

export const ProducerOutputRpc = withDullRpc({
    key:         ["derivean", "producer", "output"],
    schema:      ProducerOutputSchema,
    invalidator: [
        withDependenciesQuery.key,
        withProductionTimeQuery.key,
    ],
});
export type ProducerOutputRpc = typeof ProducerOutputRpc;
