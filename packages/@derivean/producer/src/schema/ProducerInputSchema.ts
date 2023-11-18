import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const ProducerInputSchema = withSchema({
    entity:  identityOf(z => z.object({
        producerId: z.string,
        /**
         * Item on input
         */
        itemId: z.string,
        /**
         * Amount of processed item (subtracted by time from an inventory)
         */
        amount: z.number,
    })),
    shape:   schema(z => z.object({
        producerId: z.string,
        itemId: z.string,
        amount:     z.number,
    })),
    filter:  filterOf(z => z.object({
        producerId: z.string$,
        itemId: z.string$,
    })),
    orderBy: orderByOf(["producerId", "amount"]),
});
export type ProducerInputSchema = typeof ProducerInputSchema;
