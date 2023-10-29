import {
    type PicoSchema,
    schema
}                             from "@use-pico2/schema";
import {ProducerInputSchema}  from "./ProducerInputSchema";
import {ProducerOutputSchema} from "./ProducerOutputSchema";

export const ProducerSchema = schema(z => z.object({
    input:  z.array(ProducerInputSchema),
    output: z.array(ProducerOutputSchema),
    /**
     * Runtime of a producer before resources can be picked up
     */
    time: z.number,
}));
export type ProducerSchema = typeof ProducerSchema;
export namespace ProducerSchema {
    export type Type = PicoSchema.Output<ProducerSchema>;
}
