import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const ProducerOutputSchema = identityOf(z => z.object({
    resourceId: z.string,
    amount:     z.number,
}));
export type ProducerOutputSchema = typeof ProducerOutputSchema;
export namespace ProducerOutputSchema {
    export type Type = PicoSchema.Output<ProducerOutputSchema>;
}
