import {withService}             from "@use-pico/server";
import {type BuildingRepository} from "../repository/BuildingRepository";

export const withBuildingRepository = withService<BuildingRepository.Type>("@derivean/building/BuildingRepository");
