import {withDullRpc}         from "@use-pico/dull-stuff";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export const ProducerInputRpc = withDullRpc({
    key:    ["derivean", "producer", "input"],
    schema: ProducerInputSchema,
});
export type ProducerInputRpc = typeof ProducerInputRpc;
