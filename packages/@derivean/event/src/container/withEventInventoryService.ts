import {withService}                 from "@use-pico/server";
import {type IEventInventoryService} from "../api/IEventInventoryService";

export const withEventInventoryService = withService<IEventInventoryService>("@derivean/event/EventInventoryService");
