import {withDullUI}               from "@use-pico/dull-stuff";
import {ProducerOutputQueryStore} from "../query/ProducerOutputQueryStore";
import {ProducerOutputRpc}        from "../rpc/ProducerOutputRpc";

export const ProducerOutputUI = withDullUI({
    rpc:        ProducerOutputRpc,
    queryStore: ProducerOutputQueryStore,
});
export type ProducerOutputUI = typeof ProducerOutputUI;
