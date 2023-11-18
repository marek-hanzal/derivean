import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const ItemSchema = withSchema({
    entity:  identityOf(z => z.object({
        name:   z.string,
        typeId: z.string,
    })),
    shape:   schema(z => z.object({
        name:   z.string,
        typeId: z.string,
    })),
    filter:  filterOf(z => z.object({
        name:   z.string$,
        typeId: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type ItemSchema = typeof ItemSchema;
