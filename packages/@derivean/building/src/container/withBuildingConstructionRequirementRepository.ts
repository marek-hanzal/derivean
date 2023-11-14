import {withService}                               from "@use-pico/container";
import {BuildingConstructionRequirementRepository} from "../repository/BuildingConstructionRequirementRepository";

export const withBuildingConstructionRequirementRepository = withService<BuildingConstructionRequirementRepository.Type>("@derivean/building/BuildingConstructionRequirementRepository");
