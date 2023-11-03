import {orderByOf}       from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const BuildingOrderBySchema = orderByOf(["name"]);
export type BuildingOrderBySchema = typeof BuildingOrderBySchema;
export namespace BuildingOrderBySchema {
    export type Type = PicoSchema.Output<BuildingOrderBySchema>;
}
