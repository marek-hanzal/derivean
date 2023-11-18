import {withService}       from "@use-pico/server";
import {type IHeroService} from "../api/IHeroService";

export const withHeroService = withService<IHeroService>("@derivean/hero/HeroService");
export type withHeroService = typeof withHeroService["service"];
