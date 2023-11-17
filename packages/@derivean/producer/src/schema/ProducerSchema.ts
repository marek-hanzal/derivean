import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const ProducerSchema = withSchema({
    entity:  identityOf(z => z.object({
        name: z.string,
        /**
         * Runtime of a producer before items can be picked up
         */
        time: z.number,
    })),
    shape:   schema(z => z.object({
        name: z.string,
        time: z.number,
    })),
    filter:  filterOf(z => z.object({
        name: z.string$,
    })),
    orderBy: orderByOf(["name"]),
});
export type ProducerSchema = typeof ProducerSchema;
