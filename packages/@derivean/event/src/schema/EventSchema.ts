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
        type:    z.string,
        from:     z.string$,
        to:       z.string$,
        duration: z.number$,
        userId:   z.string$,
        instant: z.bool,
        commit:  z.bool,
    })),
    shape:   schema(z => z.object({
        name:     z.string,
        type:     z.string,
        from:     z.string$,
        to:       z.string$,
        duration: z.number$,
        userId:   z.string$,
        instant:  z.bool,
    })),
    filter:  filterOf(z => z.object({
        name:    z.string$,
        type:    z.string$,
        userId:  z.string$,
        instant: z.bool$,
        commit: z.bool$,
    })),
    orderBy: orderByOf(["from", "name"]),
});
export type EventSchema = typeof EventSchema;
