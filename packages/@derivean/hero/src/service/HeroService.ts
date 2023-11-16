import {
    allFakers,
    faker
}                          from "@faker-js/faker";
import {type IHeroService} from "../api/IHeroService";

export class HeroService implements IHeroService {
    public async random(): Promise<IHeroService.RandomResult> {
        const $faker = faker.helpers.objectValue(allFakers);
        return {
            hero:  {
                name: $faker.name.fullName(),
                health:   100,
                level:    0,
                prestige: 0,
            },
            perks: [],
        };
    }
}
