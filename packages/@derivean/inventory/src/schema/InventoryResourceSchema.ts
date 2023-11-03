import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const InventoryResourceSchema = identityOf(z => z.object({
    resourceId: z.string,
    amount:     z.number,
}));
export type InventoryResourceSchema = typeof InventoryResourceSchema;
export namespace InventoryResourceSchema {
    export type Type = PicoSchema.Output<InventoryResourceSchema>;
}
