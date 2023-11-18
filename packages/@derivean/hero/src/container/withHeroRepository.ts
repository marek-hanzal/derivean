import {withService}         from "@use-pico/server";
import {type HeroRepository} from "../repository/HeroRepository";

export const withHeroRepository = withService<HeroRepository.Type>("@derivean/hero/HeroRepository");
