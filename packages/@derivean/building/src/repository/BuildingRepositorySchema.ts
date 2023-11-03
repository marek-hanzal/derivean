import {withRepositorySchema}   from "@use-pico/repository";
import {BuildingMutationSchema} from "../schema/BuildingMutationSchema";
import {BuildingQuerySchema}    from "../schema/BuildingQuerySchema";
import {BuildingSchema}         from "../schema/BuildingSchema";
import {BuildingShapeSchema}    from "../schema/BuildingShapeSchema";

export const BuildingRepositorySchema = withRepositorySchema({
    entity:   BuildingSchema,
    shape:    BuildingShapeSchema,
    query:    BuildingQuerySchema,
    mutation: BuildingMutationSchema,
});
export type BuildingRepositorySchema = typeof BuildingRepositorySchema;
