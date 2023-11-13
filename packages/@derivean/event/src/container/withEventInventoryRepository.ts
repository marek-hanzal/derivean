import {withService}              from "@use-pico/container";
import {EventInventoryRepository} from "../repository/EventInventoryRepository";

export const withEventInventoryRepository = withService<EventInventoryRepository.Type>("@derivean/event/EventInventoryRepository");
