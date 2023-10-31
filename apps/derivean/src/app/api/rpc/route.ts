import {container}       from "@derivean/server";
import {withRpcEndpoint} from "@use-pico/rpc-server";

const endpoint = withRpcEndpoint({
    container: container,
});

export {
    endpoint as POST,
};
