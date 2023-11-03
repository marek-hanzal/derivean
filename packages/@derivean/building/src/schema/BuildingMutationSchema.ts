import {type PicoSchema}     from "@use-pico/schema";
import {withMutationSchema}  from "@use-pico/source";
import {BuildingQuerySchema} from "./BuildingQuerySchema";
import {BuildingShapeSchema} from "./BuildingShapeSchema";

export const BuildingMutationSchema = withMutationSchema({
    shape: BuildingShapeSchema,
    query: BuildingQuerySchema,
});
export type BuildingMutationSchema = typeof BuildingMutationSchema;
export namespace BuildingMutationSchema {
    export type Type = PicoSchema.Output<BuildingMutationSchema>;
}
