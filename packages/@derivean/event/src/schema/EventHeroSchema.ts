import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const EventHeroSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        eventId: z.string,
        amount:  z.number,
    })),
    shape:   schema(z => z.object({
        eventId: z.string,
        amount:  z.number,
    })),
    filter:  filterOf(z => z.object({
        eventId: z.string$,
    })),
    orderBy: orderByOf(["amount"]),
});
export type EventHeroSchema = typeof EventHeroSchema;
