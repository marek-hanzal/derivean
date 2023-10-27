import {withRpcMutation}            from "@use-pico/rpc";
import {ResourceMutationRpcHandler} from "../rpc/ResourceMutationRpcHandler";

export const withResourceMutation = withRpcMutation(ResourceMutationRpcHandler);
