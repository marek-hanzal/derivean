import {
    type PicoSchema,
    schema
}                             from "@use-pico/schema";
import {ProducerInputSchema}  from "./ProducerInputSchema";
import {ProducerOutputSchema} from "./ProducerOutputSchema";

export const ProducerSchema = schema(z => z.object({
    input:  z.array(ProducerInputSchema),
    output: z.array(ProducerOutputSchema),
}));
export type ProducerSchema = typeof ProducerSchema;
export namespace ProducerSchema {
    export type Type = PicoSchema.Output<ProducerSchema>;
}
