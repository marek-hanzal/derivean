import {withDullRpc}     from "@use-pico/dull-stuff";
import {InventorySchema} from "../schema/InventorySchema";

export const InventoryRpc = withDullRpc({
    key:    ["derivean", "inventory"],
    schema: InventorySchema,
});
