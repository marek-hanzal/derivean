import {filterOf}        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ProducerFilterSchema = filterOf(z => z.object({
    name: z.string$,
}));
export type ProducerFilterSchema = typeof ProducerFilterSchema;
export namespace ProducerFilterSchema {
    export type Type = PicoSchema.Output<ProducerFilterSchema>;
}
