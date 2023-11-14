import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const InventoryItemSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        inventoryId: z.string,
        itemId:      z.string,
        amount:      z.number,
        limit:       z.number$,
    })),
    shape:   schema(z => z.object({
        inventoryId: z.string,
        itemId:      z.string,
        amount:      z.number,
        limit:       z.number$,
    })),
    filter:  filterOf(z => z.object({
        inventoryId: z.string$,
        itemId:      z.string$,
    })),
    orderBy: orderByOf(["id"]),
});
export type InventoryItemSchema = typeof InventoryItemSchema;