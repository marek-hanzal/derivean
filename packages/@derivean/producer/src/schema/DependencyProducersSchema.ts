import {
    type PicoSchema,
    schema
}                       from "@use-pico/schema";
import {ProducerSchema} from "./ProducerSchema";

export const DependencyProducersSchema = schema(z => z.object({
    producers: z.array(ProducerSchema.entity),
}));
export type DependencyProducersSchema = typeof DependencyProducersSchema;
export namespace DependencyProducersSchema {
    export type Type = PicoSchema.Output<DependencyProducersSchema>;
}
