import {withService}    from "@use-pico/container";
import {HeroRepository} from "../repository/HeroRepository";

export const withHeroRepository = withService<HeroRepository.Type>("@derivean/hero/HeroRepository");
