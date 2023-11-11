import {withDullUI}       from "@use-pico/dull-stuff";
import {ProducerInputRpc} from "../rpc/ProducerInputRpc";

export const ProducerInputUI = withDullUI({
    rpc: ProducerInputRpc,
});
