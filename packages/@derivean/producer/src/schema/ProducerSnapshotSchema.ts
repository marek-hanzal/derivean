import {InventorySchema} from "@derivean/inventory";
import {
    type PicoSchema,
    schema
}                        from "@use-pico/schema";

/**
 * Snapshot is a point-in-time calculation of produced/consumed resources.
 */
export const ProducerSnapshotSchema = schema(z => z.object({
    /**
     * Output inventory with processed changes (produced/consumed resources)
     */
    inventory: InventorySchema,
    /**
     * Relative values (plus/minus of processed resources)
     */
    relative: InventorySchema,
    /**
     * Tells if given a production result is on its limits
     */
    isLimit: z.bool,
}));
export type ProducerSnapshotSchema = typeof ProducerSnapshotSchema;
export namespace ProducerSnapshotSchema {
    export type Type = PicoSchema.Output<ProducerSnapshotSchema>;
}
