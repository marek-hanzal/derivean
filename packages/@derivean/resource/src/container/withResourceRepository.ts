import {withService}             from "@use-pico/container";
import {type ResourceRepository} from "../repository/ResourceRepository";

export const withResourceRepository = withService<ResourceRepository.Type>("@derivean/resource/ResourceRepository");
