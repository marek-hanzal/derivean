import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const EventInstanceSchema = withSchema({
    entity:  identityOf(z => z.object({
        eventId:   z.string,
        kingdomId: z.string,
        userId:    z.string,
        from:      z.string$,
        to:        z.string$,
        instant:   z.bool,
        commit:    z.bool,
    })),
    shape:   schema(z => z.object({
        eventId:   z.string,
        kingdomId: z.string,
        userId:    z.string,
        from:      z.string$,
        to:        z.string$,
        instant:   z.bool,
        commit:    z.bool,
    })),
    filter:  filterOf(z => z.object({
        eventId:   z.string$,
        kingdomId: z.string$,
        userId:    z.string$,
        instant:   z.bool$,
        commit:    z.bool$,
    })),
    orderBy: orderByOf(["from", "name"]),
});
export type EventInstanceSchema = typeof EventInstanceSchema;
