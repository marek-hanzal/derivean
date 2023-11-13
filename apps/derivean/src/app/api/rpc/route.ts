import {
    container,
    withContext
}                        from "@derivean/server";
import {withRpcEndpoint} from "@use-pico/rpc-server";

const endpoint = withRpcEndpoint({
    container,
    context: withContext,
});

export {
    endpoint as POST,
};
