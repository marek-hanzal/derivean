import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {BuildingRepository}     from "../repository/BuildingRepository";
import {BuildingRpc}            from "../rpc/BuildingRpc";
import {withBuildingRepository} from "./withBuildingRepository";

export const withBuildingContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     BuildingRepository,
        withRepository: withBuildingRepository,
        handler: BuildingRpc,
    });
};
