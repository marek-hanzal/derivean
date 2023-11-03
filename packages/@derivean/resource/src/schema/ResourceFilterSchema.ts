import {filterOf}        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ResourceFilterSchema = filterOf(z => z.object({
    name:   z.string$,
    typeId: z.string$,
}));
export type ResourceFilterSchema = typeof ResourceFilterSchema;
export namespace ResourceFilterSchema {
    export type Type = PicoSchema.Output<ResourceFilterSchema>;
}
