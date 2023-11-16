import {
    allFakers,
    faker
}                          from "@faker-js/faker";
import {type IHeroService} from "../api/IHeroService";

export class HeroService implements IHeroService {
    public async random(): Promise<IHeroService.RandomResult> {
        const $faker = allFakers[faker.helpers.arrayElement(Object.keys(allFakers)) as keyof allFakers];
        return {
            hero:  {
                name: $fake.names.name(),
                health:   100,
                level:    0,
                prestige: 0,
            },
            perks: [],
        };
    }
}
