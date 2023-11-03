import {filterOf}        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const BuildingFilterSchema = filterOf(z => z.object({
    name: z.string$,
}));
export type BuildingFilterSchema = typeof BuildingFilterSchema;
export namespace BuildingFilterSchema {
    export type Type = PicoSchema.Output<BuildingFilterSchema>;
}
