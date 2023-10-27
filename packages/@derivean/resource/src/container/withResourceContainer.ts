import {type IContainer}        from "@use-pico/container";
import {withRpcIndexService}    from "@use-pico/rpc";
import {ResourceRepository}     from "../repository/ResourceRepository";
import {ResourceMutationRpc}    from "../rpc/ResourceMutationRpc";
import {withResourceRepository} from "./withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withResourceRepository.bind(container, ResourceRepository);
    withRpcIndexService.use(container)
        .using([
            ResourceMutationRpc,
        ]);
};
