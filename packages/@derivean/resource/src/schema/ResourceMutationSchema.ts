import {type PicoSchema}     from "@use-pico2/schema";
import {withMutationSchema}  from "@use-pico2/source";
import {ResourceQuerySchema} from "./ResourceQuerySchema";
import {ResourceShapeSchema} from "./ResourceShapeSchema";

export const ResourceMutationSchema = withMutationSchema({
    shape: ResourceShapeSchema,
    query: ResourceQuerySchema,
});
export type ResourceMutationSchema = typeof ResourceMutationSchema;
export namespace ResourceMutationSchema {
    export type Type = PicoSchema.Output<ResourceMutationSchema>;
}
