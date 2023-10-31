import {withRepositorySchema}   from "@use-pico/repository";
import {ResourceMutationSchema} from "../schema/ResourceMutationSchema";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {ResourceShapeSchema}    from "../schema/ResourceShapeSchema";

export const ResourceRepositorySchema = withRepositorySchema({
    entity:   ResourceSchema,
    shape:    ResourceShapeSchema,
    query:    ResourceQuerySchema,
    mutation: ResourceMutationSchema,
});
export type ResourceRepositorySchema = typeof ResourceRepositorySchema;
