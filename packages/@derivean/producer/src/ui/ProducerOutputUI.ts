import {withDullUI}               from "@use-pico/dull-stuff";
import {ProducerOutputRpc}        from "../rpc/ProducerOutputRpc";
import {ProducerOutputQueryStore} from "../store/ProducerOutputQueryStore";

export const ProducerOutputUI = withDullUI({
    rpc:        ProducerOutputRpc,
    queryStore: ProducerOutputQueryStore,
});
export type ProducerOutputUI = typeof ProducerOutputUI;
