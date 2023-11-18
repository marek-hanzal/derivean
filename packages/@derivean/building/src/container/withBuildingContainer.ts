import {type IContainer}                               from "@use-pico/server";
import {BuildingConstructionRequirementRepository}     from "../repository/BuildingConstructionRequirementRepository";
import {BuildingRepository}                            from "../repository/BuildingRepository";
import {BuildingRequirementRepository}                 from "../repository/BuildingRequirementRepository";
import {BuildingConstructionRequirementRpc}            from "../rpc/BuildingConstructionRequirementRpc";
import {BuildingRequirementRpc}                        from "../rpc/BuildingRequirementRpc";
import {BuildingRpc}                                   from "../rpc/BuildingRpc";
import {ConstructionService}                           from "../service/ConstructionService";
import {withBuildingConstructionRequirementRepository} from "./withBuildingConstructionRequirementRepository";
import {withBuildingRepository}                        from "./withBuildingRepository";
import {withBuildingRequirementRepository}             from "./withBuildingRequirementRepository";
import {withConstructionService}                       from "./withConstructionService";

export const withBuildingContainer: IContainer.Register = container => {
    withConstructionService.bind(container, ConstructionService);
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
