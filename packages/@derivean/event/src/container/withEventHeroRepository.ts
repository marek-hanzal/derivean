import {withService}         from "@use-pico/container";
import {EventHeroRepository} from "../repository/EventHeroRepository";

export const withEventHeroRepository = withService<EventHeroRepository.Type>("@derivean/event/EventHeroRepository");
