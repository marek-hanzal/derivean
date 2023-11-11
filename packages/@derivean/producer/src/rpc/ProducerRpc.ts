import {withDullRpc}    from "@use-pico/dull-stuff";
import {ProducerSchema} from "../schema/ProducerSchema";

export const ProducerRpc = withDullRpc({
    key:    ["derivean", "producer"],
    schema: ProducerSchema,
});
export type ProducerRpc = typeof ProducerRpc;
