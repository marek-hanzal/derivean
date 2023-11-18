import {withService}                        from "@use-pico/server";
import {type BuildingRequirementRepository} from "../repository/BuildingRequirementRepository";

export const withBuildingRequirementRepository = withService<BuildingRequirementRepository.Type>("@derivean/building/BuildingRequirementRepository");
export type withBuildingRequirementRepository = typeof withBuildingRequirementRepository["service"];
