import {withComponents}           from "@use-pico/extras";
import {ProducerOutputRpc}        from "../rpc/ProducerOutputRpc";
import {ProducerOutputQueryStore} from "../store/ProducerOutputQueryStore";

export const {
    Fetch:      ProducerOutputFetch,
    Collection: ProducerOutputCollection,
} = withComponents({
    rpc:        ProducerOutputRpc,
    queryStore: ProducerOutputQueryStore,
});
