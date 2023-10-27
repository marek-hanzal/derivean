import {sortOf}           from "@use-pico/query";
import {type PicoSchema}  from "@use-pico/schema";
import {withSourceSchema} from "@use-pico/source";

export const ResourceSourceSchema = withSourceSchema(schema => ({
    entity:  schema(z => z.object({
        name: z.string,
    })),
    shape:   schema(z => z.object({
        name: z.string,
    })),
    filter:  schema(z => z.object({
        name: z.string$,
    })),
    orderBy: sortOf(["name"]),
}));

export type ResourceSourceSchema = typeof ResourceSourceSchema;
export namespace ResourceSourceSchema {
    export type Type = PicoSchema.Output<ResourceSourceSchema>;
}
