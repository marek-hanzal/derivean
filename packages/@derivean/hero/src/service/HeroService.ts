import Fakerator           from "fakerator";
import {type IHeroService} from "../api/IHeroService";

export class HeroService implements IHeroService {
    public async random(): Promise<IHeroService.RandomResult> {
        const fakerator = Fakerator("en-US");
        return {
            hero:  {
                name:     Fakerator(
                    fakerator.random.arrayElement([
                        "en-US",
                        "cs-CZ",
                        "de-DE",
                        "sv-SE",
                        "it-IT",
                        "hu-HU",
                    ])
                ).names.name(),
                health:   100,
                level:    0,
                prestige: 0,
            },
            perks: [],
        };
    }
}
