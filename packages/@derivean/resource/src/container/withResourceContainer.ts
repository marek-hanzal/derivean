import {type IContainer}     from "@use-pico/container";
import {withRpcIndexService} from "@use-pico/rpc";
import {ResourceMutationRpc} from "../rpc/ResourceMutationRpc";

export const withResourceContainer: IContainer.Register = container => {
    withRpcIndexService.resolve(container)
        .using([
            ResourceMutationRpc,
        ]);
};
