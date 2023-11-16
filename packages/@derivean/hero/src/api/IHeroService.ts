import {withDullSchema} from "@use-pico/dull-stuff";
import {HeroSchema}     from "../schema/HeroSchema";

export interface IHeroService {
    /**
     * Generate random hero
     */
    random(): Promise<IHeroService.RandomResult>;
}

export namespace IHeroService {
    export interface RandomResult {
        hero: Pick<withDullSchema.Infer.Shape<HeroSchema>, "name" | "health" | "level" | "prestige">;
        /**
         * @TODO perks not yet implemented
         */
        perks: [];
    }
}
