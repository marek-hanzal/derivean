import {withRpc}        from "@use-pico/extras";
import {ProducerSchema} from "../schema/ProducerSchema";

export const ProducerRpc = withRpc({
    key:    ["derivean", "producer"],
    schema: ProducerSchema,
});
export type ProducerRpc = typeof ProducerRpc;
