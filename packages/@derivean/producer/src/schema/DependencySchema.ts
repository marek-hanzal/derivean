import {
    type PicoSchema,
    schema
}                                  from "@use-pico/schema";
import {DependencyCycleSchema}     from "./DependencyCycleSchema";
import {DependencyProducersSchema} from "./DependencyProducersSchema";

export const DependencySchema = schema(z => z.union([
    DependencyProducersSchema,
    DependencyCycleSchema,
]));
export type DependenciesSchema = typeof DependencySchema;
export namespace DependenciesSchema {
    export type Type = PicoSchema.Output<DependenciesSchema>;
}
