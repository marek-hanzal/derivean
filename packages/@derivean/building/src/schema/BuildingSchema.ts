import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const BuildingSchema = withSchema({
    entity:  identityOf(z => z.object({
        name:         z.string,
        construction: z.number,
        maximum:      z.number,
    })),
    shape:   schema(z => z.object({
        name:         z.string,
        construction: z.number,
        maximum:      z.number,
    })),
    filter:  filterOf(z => z.object({
        name: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type BuildingSchema = typeof BuildingSchema;
