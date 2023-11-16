import {withService}            from "@use-pico/container";
import {type IEventHeroService} from "../api/IEventHeroService";

export const withEventHeroService = withService<IEventHeroService>("@derivean/event/EventHeroService");
