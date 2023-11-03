import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const ProducerInputSchema = identityOf(z => z.object({
    /**
     * Resource on input
     */
    resourceId: z.string,
    /**
     * Amount of processed resource (subtracted by time from an inventory)
     */
    amount: z.number,
}));
export type ProducerInputSchema = typeof ProducerInputSchema;
export namespace ProducerInputSchema {
    export type Type = PicoSchema.Output<ProducerInputSchema>;
}
