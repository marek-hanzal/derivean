import {filterOf}        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ProducerInputFilterSchema = filterOf(z => z.object({
    producerId: z.string$,
    resourceId: z.string$,
}));
export type ProducerInputFilterSchema = typeof ProducerInputFilterSchema;
export namespace ProducerInputFilterSchema {
    export type Type = PicoSchema.Output<ProducerInputFilterSchema>;
}
