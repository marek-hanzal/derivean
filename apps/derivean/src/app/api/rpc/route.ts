import {container}       from "@/derivean/container";
import {withRpcEndpoint} from "@use-pico/rpc";

const endpoint = withRpcEndpoint({
    container,
});

export {
    endpoint as POST
};
