import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const BuildingSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        name:       z.string,
        producerId: z.string,
    })),
    shape:   schema(z => z.object({
        name:       z.string,
        producerId: z.string$,
    })),
    filter:  filterOf(z => z.object({
        name:       z.string$,
        producerId: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type BuildingSchema = typeof BuildingSchema;
