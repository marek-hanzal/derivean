import {withService}          from "@use-pico/server";
import {type EventRepository} from "../repository/EventRepository";

export const withEventRepository = withService<EventRepository.Type>("@derivean/event/EventRepository");
