import {ResourceSchema} from "@derivean/resource";
import {
    type PicoSchema,
    schema
}                       from "@use-pico/schema";

export const InventoryResourceSchema = schema(z => z.object({
    resource: ResourceSchema,
    amount:   z.number,
}));
export type InventoryResourceSchema = typeof InventoryResourceSchema;
export namespace InventoryResourceSchema {
    export type Type = PicoSchema.Output<InventoryResourceSchema>;
}
