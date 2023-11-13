import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const ItemTypeSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        name: z.string,
    })),
    shape:   schema(z => z.object({
        name: z.string,
    })),
    filter:  filterOf(z => z.object({
        name: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type ItemTypeSchema = typeof ItemTypeSchema;
