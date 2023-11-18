import {withComponents}          from "@use-pico/extras";
import {ProducerInputRpc}        from "../rpc/ProducerInputRpc";
import {ProducerInputQueryStore} from "../store/ProducerInputQueryStore";

export const {
    Fetch:      ProducerInputFetch,
    Collection: ProducerInputCollection,
} = withComponents({
    rpc:        ProducerInputRpc,
    queryStore: ProducerInputQueryStore,
});
