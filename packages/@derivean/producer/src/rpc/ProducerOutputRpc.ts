import {withDullRpc}          from "@use-pico/dull-stuff";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export const ProducerOutputRpc = withDullRpc({
    key:    ["derivean", "producer", "output"],
    schema: ProducerOutputSchema,
});
export type ProducerOutputRpc = typeof ProducerOutputRpc;
