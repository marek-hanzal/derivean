import {
    type PicoSchema,
    schema
}                       from "@use-pico/schema";
import {ProducerSchema} from "./ProducerSchema";

export const DependencyCycleSchema = schema(z => z.object({
    cycle: z.array(ProducerSchema.entity),
}));
export type DependencyCycleSchema = typeof DependencyCycleSchema;
export namespace DependencyCycleSchema {
    export type Type = PicoSchema.Output<DependencyCycleSchema>;
}
