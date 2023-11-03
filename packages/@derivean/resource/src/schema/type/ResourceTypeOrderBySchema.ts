import {orderByOf}       from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ResourceTypeOrderBySchema = orderByOf(["name"]);
export type ResourceTypeOrderBySchema = typeof ResourceTypeOrderBySchema;
export namespace ResourceTypeOrderBySchema {
    export type Type = PicoSchema.Output<ResourceTypeOrderBySchema>;
}
