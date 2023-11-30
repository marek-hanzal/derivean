import {withComponents}      from "@use-pico/extras";
import {EventHeroRpc}        from "../rpc/EventHeroRpc";
import {EventHeroQueryStore} from "../store/EventHeroQueryStore";

export const EventHeroComponents = withComponents({
    rpc:        EventHeroRpc,
    queryStore: EventHeroQueryStore,
});
export type EventHeroComponents = typeof EventHeroComponents;
