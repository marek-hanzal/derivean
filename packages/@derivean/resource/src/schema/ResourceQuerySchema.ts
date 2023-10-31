import {withQuerySchema}       from "@use-pico/query";
import {type PicoSchema}       from "@use-pico/schema";
import {ResourceFilterSchema}  from "./ResourceFilterSchema";
import {ResourceOrderBySchema} from "./ResourceOrderBySchema";

export const ResourceQuerySchema = withQuerySchema({
    filter:  ResourceFilterSchema,
    orderBy: ResourceOrderBySchema,
});
export type ResourceQuerySchema = typeof ResourceQuerySchema;
export namespace ResourceQuerySchema {
    export type Type = PicoSchema.Output<ResourceQuerySchema>;
}
