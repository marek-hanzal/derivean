import {ResourceSchema} from "@derivean/resource";
import {
    type PicoSchema,
    schema
}                       from "@use-pico/schema";

export const ProducerOutputSchema = schema(z => z.object({
    resource: ResourceSchema,
    amount:   z.number,
    time:     z.number,
}));
export type ProducerOutputSchema = typeof ProducerOutputSchema;
export namespace ProducerOutputSchema {
    export type Type = PicoSchema.Output<ProducerOutputSchema>;
}
