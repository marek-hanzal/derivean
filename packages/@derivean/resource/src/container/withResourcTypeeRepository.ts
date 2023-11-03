import {withService}                 from "@use-pico/container";
import {type ResourceTypeRepository} from "../repository/ResourceTypeRepository";

export const withResourceTypeRepository = withService<ResourceTypeRepository.Type>("@derivean/resource/ResourceTypeRepository");
