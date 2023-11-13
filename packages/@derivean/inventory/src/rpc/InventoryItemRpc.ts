import {withDullRpc}         from "@use-pico/dull-stuff";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export const InventoryItemRpc = withDullRpc({
    key:    ["derivean", "inventory", "item"],
    schema: InventoryItemSchema,
});
