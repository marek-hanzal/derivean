import {withQuerySchema}       from "@use-pico/query";
import {type PicoSchema}       from "@use-pico/schema";
import {BuildingFilterSchema}  from "./BuildingFilterSchema";
import {BuildingOrderBySchema} from "./BuildingOrderBySchema";

export const BuildingQuerySchema = withQuerySchema({
    filter:  BuildingFilterSchema,
    orderBy: BuildingOrderBySchema,
});
export type BuildingQuerySchema = typeof BuildingQuerySchema;
export namespace BuildingQuerySchema {
    export type Type = PicoSchema.Output<BuildingQuerySchema>;
}
