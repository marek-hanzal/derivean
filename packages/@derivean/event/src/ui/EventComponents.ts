import {withComponents}  from "@use-pico/extras";
import {EventRpc}        from "../rpc/EventRpc";
import {EventQueryStore} from "../store/EventQueryStore";

export const EventComponents = withComponents({
    rpc:        EventRpc,
    queryStore: EventQueryStore,
});
