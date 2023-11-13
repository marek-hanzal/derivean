import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const ProducerOutputSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        producerId: z.string,
        itemId: z.string,
        amount:     z.number,
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
    orderBy: orderByOf(["itemId", "amount"]),
});
export type ProducerOutputSchema = typeof ProducerOutputSchema;
