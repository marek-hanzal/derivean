import {type Infer} from "@use-pico/extras";
import {HeroSchema} from "../schema/HeroSchema";

export interface IHeroService {
    /**
     * Generate random hero
     */
    random(): Promise<IHeroService.RandomResult>;
}

export namespace IHeroService {
    export interface RandomResult {
        hero: Pick<Infer.Shape<HeroSchema>, "name" | "health" | "level" | "prestige">;
        /**
         * @TODO perks not yet implemented
         */
        perks: [];
    }
}
