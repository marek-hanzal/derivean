import {withRepositorySchema}   from "@use-pico/repository";
import {BuildingMutationSchema} from "./BuildingMutationSchema";
import {BuildingQuerySchema}    from "./BuildingQuerySchema";
import {BuildingSchema}         from "./BuildingSchema";
import {BuildingShapeSchema}    from "./BuildingShapeSchema";

export const BuildingRepositorySchema = withRepositorySchema({
    entity:   BuildingSchema,
    shape:    BuildingShapeSchema,
    query:    BuildingQuerySchema,
    mutation: BuildingMutationSchema,
});
export type BuildingRepositorySchema = typeof BuildingRepositorySchema;
