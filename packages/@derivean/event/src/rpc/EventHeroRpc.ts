import {EventHeroSchema} from "../schema/EventHeroSchema";

export const EventHeroRpc = withDullRpc({
    key:    ["derivean", "event", "hero"],
    schema: EventHeroSchema,
});
