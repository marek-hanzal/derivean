import {withMutation}        from "@use-pico/rpc";
import {ResourceMutationRpc} from "../rpc/ResourceMutationRpc";

export const withResourceMutation = withMutation({
    service: ResourceMutationRpc.key,
    schema:  {
        // request:
    }
});
