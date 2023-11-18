import {withService}         from "@use-pico/server";
import {EventHeroRepository} from "../repository/EventHeroRepository";

export const withEventHeroRepository = withService<EventHeroRepository.Type>("@derivean/event/EventHeroRepository");
export type withEventHeroRepository = typeof withEventHeroRepository["service"];
