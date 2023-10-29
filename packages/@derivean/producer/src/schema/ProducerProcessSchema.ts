import {InventorySchema} from "@derivean/inventory";
import {
    type PicoSchema,
    schema
}                        from "@use-pico2/schema";
import {ProducerSchema}  from "./ProducerSchema";

export const ProducerProcessSchema = schema(z => z.object({
    /**
     * Current inventory
     */
    inventory: InventorySchema,
    /**
     * Producer definition a computation is based on
     */
    producer: ProducerSchema,
    /**
     * Date where production computation should happen
     */
    date: z.object({
        from: z.string,
        to:   z.string,
    }),
}));
export type ProducerProcessSchema = typeof ProducerProcessSchema;
export namespace ProducerProcessSchema {
    export type Type = PicoSchema.Output<ProducerProcessSchema>;
}
