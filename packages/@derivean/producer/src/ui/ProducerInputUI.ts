import {withDullUI}              from "@use-pico/dull-stuff";
import {ProducerInputQueryStore} from "../query/ProducerInputQueryStore";
import {ProducerInputRpc}        from "../rpc/ProducerInputRpc";

export const ProducerInputUI = withDullUI({
    rpc:        ProducerInputRpc,
    queryStore: ProducerInputQueryStore,
});
