import {withDullUI}          from "@use-pico/dull-stuff";
import {EventHeroQueryStore} from "../query/EventHeroQueryStore";
import {EventHeroRpc}        from "../rpc/EventHeroRpc";

export const EventHeroUI = withDullUI({
    rpc:        EventHeroRpc,
    queryStore: EventHeroQueryStore,
});
export type EventHeroUI = typeof EventHeroUI;
