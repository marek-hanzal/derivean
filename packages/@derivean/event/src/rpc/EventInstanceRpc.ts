import {withDullRpc}         from "@use-pico/dull-stuff";
import {EventInstanceSchema} from "../schema/EventInstanceSchema";

export const EventInstanceRpc = withDullRpc({
    key:    ["derivean", "event", "instance"],
    schema: EventInstanceSchema,
});
