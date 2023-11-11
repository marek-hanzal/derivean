import {type IContainer}            from "@use-pico/container";
import {withRepositoryHandler}      from "@use-pico/rpc-server";
import {ResourceRepository}         from "../repository/ResourceRepository";
import {ResourceTypeRepository}     from "../repository/ResourceTypeRepository";
import {ResourceRpc}                from "../rpc/ResourceRpc";
import {ResourceTypeRpc}            from "../rpc/ResourceTypeRpc";
import {withResourceRepository}     from "./withResourceRepository";
import {withResourceTypeRepository} from "./withResourcTypeeRepository";

export const withResourceContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     ResourceRepository,
        withRepository: withResourceRepository,
        handler: ResourceRpc,
    });
    withRepositoryHandler({
        container,
        repository:     ResourceTypeRepository,
        withRepository: withResourceTypeRepository,
        handler: ResourceTypeRpc,
    });
};
