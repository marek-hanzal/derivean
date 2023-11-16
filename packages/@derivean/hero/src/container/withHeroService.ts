import {withService}       from "@use-pico/container";
import {type IHeroService} from "../api/IHeroService";

export const withHeroService = withService<IHeroService>("@derivean/hero/HeroService");
