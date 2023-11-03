import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ProducerInputShapeSchema = schema(z => z.object({
    producerId: z.string,
    resourceId: z.string,
    amount:     z.number,
}));
export type ProducerInputShapeSchema = typeof ProducerInputShapeSchema;
export namespace ProducerInputShapeSchema {
    export type Type = PicoSchema.Output<ProducerInputShapeSchema>;
}
