import {withService}                  from "@use-pico/container";
import {type EventInstanceRepository} from "../repository/EventInstanceRepository";

export const withEventInstanceRepository = withService<EventInstanceRepository.Type>("@derivean/event/EventInstanceRepository");
