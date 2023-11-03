import {withService}             from "@use-pico/container";
import {type BuildingRepository} from "../repository/BuildingRepository";

export const withBuildingRepository = withService<BuildingRepository.Type>("@derivean/building/Repository");
