import {ResourceSchema} from "@derivean/resource";
import {
    type PicoSchema,
    schema
}                       from "@use-pico/schema";

export const ProducerInputSchema = schema(z => z.object({
    /**
     * Resource on input
     */
    resource: ResourceSchema,
    /**
     * Amount of processed resource (subtracted by time from an inventory)
     */
    amount: z.number,
}));
export type ProducerInputSchema = typeof ProducerInputSchema;
export namespace ProducerInputSchema {
    export type Type = PicoSchema.Output<ProducerInputSchema>;
}
