import {
    allFakers,
    faker
}                          from "@faker-js/faker";
import {type IHeroService} from "../api/IHeroService";

export class HeroService implements IHeroService {
    public async random(): Promise<IHeroService.RandomResult> {
        const $faker = allFakers[faker.helpers.arrayElement([
            "cs_CZ",
            "da",
            "de",
            "en",
            "fr",
            "it",
            "pl",
            "sv",
        ]) as keyof typeof allFakers];
        return {
            hero:  {
                name: $faker.person.fullName(),
                health:   100,
                level:    0,
                prestige: 0,
            },
            perks: [],
        };
    }
}
