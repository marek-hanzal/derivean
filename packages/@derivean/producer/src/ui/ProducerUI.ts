import {withDullUI}         from "@use-pico/dull-stuff";
import {ProducerQueryStore} from "../query/ProducerQueryStore";
import {ProducerRpc}        from "../rpc/ProducerRpc";

export const ProducerUI = withDullUI({
    rpc:        ProducerRpc,
    queryStore: ProducerQueryStore,
});
