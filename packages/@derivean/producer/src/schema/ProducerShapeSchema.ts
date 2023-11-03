import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ProducerShapeSchema = schema(z => z.object({
    name: z.string,
    time: z.number,
}));
export type ProducerShapeSchema = typeof ProducerShapeSchema;
export namespace ProducerShapeSchema {
    export type Type = PicoSchema.Output<ProducerShapeSchema>;
}
