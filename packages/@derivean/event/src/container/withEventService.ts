import {withService}   from "@use-pico/container";
import {IEventService} from "../api/IEventService";

export const withEventService = withService<IEventService>("@derivean/event/EventService");
