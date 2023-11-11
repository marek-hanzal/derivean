import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const InventorySchema = withDullSchema({
    entity:  identityOf(z => z.object({})),
    shape:   schema(z => z.object({})),
    filter:  filterOf(z => z.object({})),
    orderBy: orderByOf(["id"]),
});
export type InventorySchema = typeof InventorySchema;
