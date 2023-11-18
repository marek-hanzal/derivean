import {withRpc}         from "@use-pico/extras";
import {InventorySchema} from "../schema/InventorySchema";

export const InventoryRpc = withRpc({
    key:    ["derivean", "inventory"],
    schema: InventorySchema,
});
