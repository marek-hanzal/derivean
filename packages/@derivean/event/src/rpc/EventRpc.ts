import {withDullRpc} from "@use-pico/dull-stuff";
import {EventSchema} from "../schema/EventSchema";

export const EventRpc = withDullRpc({
    key:    ["derivean", "event"],
    schema: EventSchema,
});
