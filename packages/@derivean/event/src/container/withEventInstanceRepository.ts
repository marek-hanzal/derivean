import {withService}                  from "@use-pico/server";
import {type EventInstanceRepository} from "../repository/EventInstanceRepository";

export const withEventInstanceRepository = withService<EventInstanceRepository.Type>("@derivean/event/EventInstanceRepository");
