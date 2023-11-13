import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const EventInventorySchema = withDullSchema({
    entity:  identityOf(z => z.object({
        eventId:     z.string,
        inventoryId: z.string,
    })),
    shape:   schema(z => z.object({
        eventId:     z.string,
        inventoryId: z.string,
    })),
    filter:  filterOf(z => z.object({
        eventId:     z.string$,
        inventoryId: z.string$,
    })),
    orderBy: orderByOf(["eventId"]),
});
export type EventInventorySchema = typeof EventInventorySchema;
