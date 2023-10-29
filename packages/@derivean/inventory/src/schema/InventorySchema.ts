import {
    type PicoSchema,
    schema
}                                from "@use-pico2/schema";
import {InventoryResourceSchema} from "./InventoryResourceSchema";

export const InventorySchema = schema(z => z.object({
    resources: z.array(InventoryResourceSchema),
}));
export type InventorySchema = typeof InventorySchema;
export namespace InventorySchema {
    export type Type = PicoSchema.Output<InventorySchema>;
}
