import {withService}        from "@use-pico/server";
import {type IEventService} from "../api/IEventService";

export const withEventService = withService<IEventService>("@derivean/event/EventService");
