import {filterOf}        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ResourceTypeFilterSchema = filterOf(z => z.object({
    name: z.string$,
}));
export type ResourceTypeFilterSchema = typeof ResourceTypeFilterSchema;
export namespace ResourceTypeFilterSchema {
    export type Type = PicoSchema.Output<ResourceTypeFilterSchema>;
}
