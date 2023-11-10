import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const ProducerInputSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        producerId: z.string,
        /**
         * Resource on input
         */
        resourceId: z.string,
        /**
         * Amount of processed resource (subtracted by time from an inventory)
         */
        amount: z.number,
    })),
    shape:   schema(z => z.object({
        producerId: z.string,
        resourceId: z.string,
        amount:     z.number,
    })),
    filter:  filterOf(z => z.object({
        producerId: z.string$,
        resourceId: z.string$,
    })),
    orderBy: orderByOf(["producerId", "amount"]),
});
export type ProducerInputSchema = typeof ProducerInputSchema;
