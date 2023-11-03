import {type PicoSchema}         from "@use-pico/schema";
import {withMutationSchema}      from "@use-pico/source";
import {ResourceTypeQuerySchema} from "./ResourceTypeQuerySchema";
import {ResourceTypeShapeSchema} from "./ResourceTypeShapeSchema";

export const ResourceTypeMutationSchema = withMutationSchema({
    shape: ResourceTypeShapeSchema,
    query: ResourceTypeQuerySchema,
});
export type ResourceTypeMutationSchema = typeof ResourceTypeMutationSchema;
export namespace ResourceTypeMutationSchema {
    export type Type = PicoSchema.Output<ResourceTypeMutationSchema>;
}
