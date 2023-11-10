import {dullSchema} from "@use-pico/dull-stuff";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const ResourceTypeSchema = dullSchema({
    entity:  identityOf(z => z.object({
        name: z.string,
    })),
    shape:   schema(z => z.object({
        name: z.string,
    })),
    filter:  schema(z => z.object({
        name: z.string$,
    })),
    orderBy: ["name"],
});
export type ResourceTypeSchema = typeof ResourceTypeSchema;
