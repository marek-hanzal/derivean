import {withRpc}         from "@use-pico/extras";
import {EventHeroSchema} from "../schema/EventHeroSchema";

export const EventHeroRpc = withRpc({
    key:    ["derivean", "event", "hero"],
    schema: EventHeroSchema,
});
