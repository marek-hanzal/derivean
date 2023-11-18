import {withRpc}             from "@use-pico/extras";
import {EventInstanceSchema} from "../schema/EventInstanceSchema";

export const EventInstanceRpc = withRpc({
    key:    ["derivean", "event", "instance"],
    schema: EventInstanceSchema,
});
