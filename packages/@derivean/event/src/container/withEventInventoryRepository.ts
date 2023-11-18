import {withService}              from "@use-pico/server";
import {EventInventoryRepository} from "../repository/EventInventoryRepository";

export const withEventInventoryRepository = withService<EventInventoryRepository.Type>("@derivean/event/EventInventoryRepository");
