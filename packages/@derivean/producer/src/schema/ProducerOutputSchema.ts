import {ResourceSchema} from "@derivean/resource";
import {
    type PicoSchema,
    schema
}                       from "@use-pico2/schema";

export const ProducerOutputSchema = schema(z => z.object({
    resource: ResourceSchema,
    amount:   z.number,
}));
export type ProducerOutputSchema = typeof ProducerOutputSchema;
export namespace ProducerOutputSchema {
    export type Type = PicoSchema.Output<ProducerOutputSchema>;
}
