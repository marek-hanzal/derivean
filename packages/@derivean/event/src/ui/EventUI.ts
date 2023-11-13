import {withDullUI}      from "@use-pico/dull-stuff";
import {EventQueryStore} from "../query/EventQueryStore";
import {EventRpc}        from "../rpc/EventRpc";

export const EventUI = withDullUI({
    rpc:        EventRpc,
    queryStore: EventQueryStore,
});
export type EventUI = typeof EventUI;
