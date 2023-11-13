import {withDullRpc}          from "@use-pico/dull-stuff";
import {EventInventorySchema} from "../schema/EventInventorySchema";

export const EventInventoryRpc = withDullRpc({
    key:    ["derivean", "event", "type", "EventInventory"],
    schema: EventInventorySchema,
});
