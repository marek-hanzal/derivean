import {InventorySchema} from "@derivean/inventory";
import {
    type PicoSchema,
    schema
}                        from "@use-pico2/schema";

/**
 * Snapshot is a point-in-time calculation of produced/consumed resources.
 */
export const ProducerSnapshotSchema = schema(z => z.object({
    /**
     * Generated inventory changes (relative); it could be merged later on
     */
    inventory: InventorySchema,
    /**
     * Tells if given a production result is on its limits
     */
    isLimit: z.bool,
    /**
     * Is there anything produced? (ready to pick up)
     */
    isReady: z.bool,
}));
export type ProducerSnapshotSchema = typeof ProducerSnapshotSchema;
export namespace ProducerSnapshotSchema {
    export type Type = PicoSchema.Output<ProducerSnapshotSchema>;
}
