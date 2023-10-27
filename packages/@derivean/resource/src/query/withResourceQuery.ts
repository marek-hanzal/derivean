import {withRpcSourceQuery}      from "@use-pico/rpc";
import {ResourceQueryRpcHandler} from "../rpc/ResourceQueryRpcHandler";

export const withResourceQuery = withRpcSourceQuery(ResourceQueryRpcHandler);
