import {withRpc}     from "@use-pico/extras";
import {EventSchema} from "../schema/EventSchema";

export const EventRpc = withRpc({
    key:    ["derivean", "event"],
    schema: EventSchema,
});
