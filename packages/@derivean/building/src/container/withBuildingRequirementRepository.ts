import {withService}                        from "@use-pico/container";
import {type BuildingRequirementRepository} from "../repository/BuildingRequirementRepository";

export const withBuildingRequirementRepository = withService<BuildingRequirementRepository.Type>("@derivean/building/BuildingRequirementRepository");
