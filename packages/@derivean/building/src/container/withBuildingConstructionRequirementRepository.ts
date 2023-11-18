import {withService}                               from "@use-pico/server";
import {BuildingConstructionRequirementRepository} from "../repository/BuildingConstructionRequirementRepository";

export const withBuildingConstructionRequirementRepository = withService<BuildingConstructionRequirementRepository.Type>("@derivean/building/BuildingConstructionRequirementRepository");
export type withBuildingConstructionRequirementRepository = typeof withBuildingConstructionRequirementRepository["service"];
