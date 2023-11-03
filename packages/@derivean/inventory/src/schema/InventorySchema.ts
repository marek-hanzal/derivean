import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const InventorySchema = identityOf(z => z.object({}));
export type InventorySchema = typeof InventorySchema;
export namespace InventorySchema {
    export type Type = PicoSchema.Output<InventorySchema>;
}
