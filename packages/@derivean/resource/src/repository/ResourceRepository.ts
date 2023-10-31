import {withRepository}         from "@use-pico/repository";
import {ResourceMutationSchema} from "../schema/ResourceMutationSchema";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {ResourceShapeSchema}    from "../schema/ResourceShapeSchema";

export const ResourceRepository = withRepository({
    schema: {
        entity:   ResourceSchema,
        shape:    ResourceShapeSchema,
        query:    ResourceQuerySchema,
        mutation: ResourceMutationSchema,
    },
    table:  "Resource",
});
