import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const InventoryResourceSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        resourceId: z.string,
        amount:     z.number,
    })),
    shape:   schema(z => z.object({
        resourceId: z.string,
        amount:     z.number,
    })),
    filter:  filterOf(z => z.object({
        resourceId: z.string$,
    })),
    orderBy: orderByOf(["id"]),
});
