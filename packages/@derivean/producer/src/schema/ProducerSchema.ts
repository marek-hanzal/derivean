import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const ProducerSchema = identityOf(z => z.object({
    name: z.string,
    /**
     * Runtime of a producer before resources can be picked up
     */
    time: z.number,
}));
export type ProducerSchema = typeof ProducerSchema;
export namespace ProducerSchema {
    export type Type = PicoSchema.Output<ProducerSchema>;
}
