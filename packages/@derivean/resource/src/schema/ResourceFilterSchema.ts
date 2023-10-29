import {filterOf}        from "@use-pico2/query";
import {type PicoSchema} from "@use-pico2/schema";

export const ResourceFilterSchema = filterOf(z => z.object({
    name: z.string$,
}));
export type ResourceFilterSchema = typeof ResourceFilterSchema;
export namespace ResourceFilterSchema {
    export type Type = PicoSchema.Output<ResourceFilterSchema>;
}
