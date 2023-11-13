import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const EventSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        name:     z.string,
        from:     z.string$,
        to:       z.string$,
        duration: z.number$,
        userId:   z.string$,
    })),
    shape:   schema(z => z.object({
        name: z.string,
    })),
    filter:  filterOf(z => z.object({
        name:   z.string$,
        userId: z.string$,
    })),
    orderBy: orderByOf(["from", "name"]),
});
export type EventSchema = typeof EventSchema;
