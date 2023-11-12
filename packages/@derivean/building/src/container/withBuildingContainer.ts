import {type IContainer}                   from "@use-pico/container";
import {withRepositoryHandler}             from "@use-pico/rpc-server";
import {BuildingRepository}                from "../repository/BuildingRepository";
import {BuildingRequirementRepository}     from "../repository/BuildingRequirementRepository";
import {BuildingRequirementRpc}            from "../rpc/BuildingRequirementRpc";
import {BuildingRpc}                       from "../rpc/BuildingRpc";
import {withBuildingRepository}            from "./withBuildingRepository";
import {withBuildingRequirementRepository} from "./withBuildingRequirementRepository";

export const withBuildingContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     BuildingRepository,
        withRepository: withBuildingRepository,
        handler: BuildingRpc,
    });
    withRepositoryHandler({
        container,
        repository:     BuildingRequirementRepository,
        withRepository: withBuildingRequirementRepository,
        handler:        BuildingRequirementRpc,
    });
};
