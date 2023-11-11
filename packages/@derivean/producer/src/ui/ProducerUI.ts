import {withDullUI}  from "@use-pico/dull-stuff";
import {ProducerRpc} from "../rpc/ProducerRpc";

export const ProducerUI = withDullUI({
    rpc: ProducerRpc,
});
