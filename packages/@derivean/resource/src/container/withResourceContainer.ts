import {type IContainer}            from "@use-pico/container";
import {withRpcIndexService}        from "@use-pico/rpc";
import {ResourceRepository}         from "../repository/ResourceRepository";
import {ResourceMutationRpcHandler} from "../rpc/ResourceMutationRpcHandler";
import {ResourceQueryRpcHandler}    from "../rpc/ResourceQueryRpcHandler";
import {withResourceRepository}     from "./withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withResourceRepository.bind(container, ResourceRepository);
    withRpcIndexService.use(container)
        .using([
            ResourceMutationRpcHandler,
            ResourceQueryRpcHandler,
        ]);
};
