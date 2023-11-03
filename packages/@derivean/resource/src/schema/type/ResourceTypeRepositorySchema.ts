import {withRepositorySchema}       from "@use-pico/repository";
import {ResourceTypeMutationSchema} from "./ResourceTypeMutationSchema";
import {ResourceTypeQuerySchema}    from "./ResourceTypeQuerySchema";
import {ResourceTypeSchema}         from "./ResourceTypeSchema";
import {ResourceTypeShapeSchema}    from "./ResourceTypeShapeSchema";

export const ResourceTypeRepositorySchema = withRepositorySchema({
    entity:   ResourceTypeSchema,
    shape:    ResourceTypeShapeSchema,
    query:    ResourceTypeQuerySchema,
    mutation: ResourceTypeMutationSchema,
});
export type ResourceTypeRepositorySchema = typeof ResourceTypeRepositorySchema;
