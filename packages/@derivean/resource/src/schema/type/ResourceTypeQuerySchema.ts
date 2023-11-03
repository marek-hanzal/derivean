import {withQuerySchema}           from "@use-pico/query";
import {type PicoSchema}           from "@use-pico/schema";
import {ResourceTypeFilterSchema}  from "./ResourceTypeFilterSchema";
import {ResourceTypeOrderBySchema} from "./ResourceTypeOrderBySchema";

export const ResourceTypeQuerySchema = withQuerySchema({
    filter:  ResourceTypeFilterSchema,
    orderBy: ResourceTypeOrderBySchema,
});
export type ResourceTypeQuerySchema = typeof ResourceTypeQuerySchema;
export namespace ResourceTypeQuerySchema {
    export type Type = PicoSchema.Output<ResourceTypeQuerySchema>;
}
