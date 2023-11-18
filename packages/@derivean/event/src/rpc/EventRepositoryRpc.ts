import {withRpc}              from "@use-pico/extras";
import {EventInventorySchema} from "../schema/EventInventorySchema";

export const EventInventoryRpc = withRpc({
    key:    ["derivean", "event", "type", "EventInventory"],
    schema: EventInventorySchema,
});
