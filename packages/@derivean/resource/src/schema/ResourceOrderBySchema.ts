import {orderByOf}       from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ResourceOrderBySchema = orderByOf(["name"]);
export type ResourceOrderBySchema = typeof ResourceOrderBySchema;
export namespace ResourceOrderBySchema {
    export type Type = PicoSchema.Output<ResourceOrderBySchema>;
}
