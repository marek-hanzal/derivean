import {withRpc}             from "@use-pico/extras";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export const InventoryItemRpc = withRpc({
    key:    ["derivean", "inventory", "item"],
    schema: InventoryItemSchema,
});
