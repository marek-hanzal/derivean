import {withService}            from "@use-pico/server";
import {type IEventHeroService} from "../api/IEventHeroService";

export const withEventHeroService = withService<IEventHeroService>("@derivean/event/EventHeroService");
