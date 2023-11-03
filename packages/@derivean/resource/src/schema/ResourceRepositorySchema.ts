import {withRepositorySchema}   from "@use-pico/repository";
import {ResourceMutationSchema} from "./ResourceMutationSchema";
import {ResourceQuerySchema}    from "./ResourceQuerySchema";
import {ResourceSchema}         from "./ResourceSchema";
import {ResourceShapeSchema}    from "./ResourceShapeSchema";

export const ResourceRepositorySchema = withRepositorySchema({
    entity: ResourceSchema,
    shape:    ResourceShapeSchema,
    query:    ResourceQuerySchema,
    mutation: ResourceMutationSchema,
});
export type ResourceRepositorySchema = typeof ResourceRepositorySchema;
