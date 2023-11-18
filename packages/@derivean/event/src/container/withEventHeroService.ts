import {withService}            from "@use-pico/server";
import {type IEventHeroService} from "../api/IEventHeroService";

export const withEventHeroService = withService<IEventHeroService>("@derivean/event/EventHeroService");
export type withEventHeroService = typeof withEventHeroService["service"];
