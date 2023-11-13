import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const KingdomSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        name:        z.string,
        inventoryId: z.string,
        userId:      z.string,
        created:     z.string,
    })),
    shape:   schema(z => z.object({
        name:        z.string,
        inventoryId: z.string$,
    })),
    filter:  filterOf(z => z.object({
        name:   z.string$,
        userId: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type KingdomSchema = typeof KingdomSchema;
