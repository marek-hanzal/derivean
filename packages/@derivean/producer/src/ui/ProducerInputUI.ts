import {withDullUI}              from "@use-pico/dull-stuff";
import {ProducerInputRpc}        from "../rpc/ProducerInputRpc";
import {ProducerInputQueryStore} from "../store/ProducerInputQueryStore";

export const ProducerInputUI = withDullUI({
    rpc:        ProducerInputRpc,
    queryStore: ProducerInputQueryStore,
});
export type ProducerInputUI = typeof ProducerInputUI;
