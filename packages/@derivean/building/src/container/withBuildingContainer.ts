import {type IContainer}                               from "@use-pico/container";
import {withRepositoryHandler}                         from "@use-pico/rpc-server";
import {BuildingConstructionRequirementRepository}     from "../repository/BuildingConstructionRequirementRepository";
import {BuildingRepository}                            from "../repository/BuildingRepository";
import {BuildingRequirementRepository}                 from "../repository/BuildingRequirementRepository";
import {BuildingConstructionRequirementRpc}            from "../rpc/BuildingConstructionRequirementRpc";
import {BuildingRequirementRpc}                        from "../rpc/BuildingRequirementRpc";
import {BuildingRpc}                                   from "../rpc/BuildingRpc";
import {withBuildingConstructionRequirementRepository} from "./withBuildingConstructionRequirementRepository";
import {withBuildingRepository}                        from "./withBuildingRepository";
import {withBuildingRequirementRepository}             from "./withBuildingRequirementRepository";

export const withBuildingContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     BuildingRepository,
        withRepository: withBuildingRepository,
        handler: BuildingRpc,
    });
    withRepositoryHandler({
        container,
        repository:     BuildingConstructionRequirementRepository,
        withRepository: withBuildingConstructionRequirementRepository,
        handler:        BuildingConstructionRequirementRpc,
    });
    withRepositoryHandler({
        container,
        repository:     BuildingRequirementRepository,
        withRepository: withBuildingRequirementRepository,
        handler:        BuildingRequirementRpc,
    });
};
