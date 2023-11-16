import {withDullSchema} from "@use-pico/dull-stuff";
import {Progress}       from "@use-pico/ui";
import {type FC}        from "react";
import {HeroSchema}     from "../schema/HeroSchema";

export namespace HeroHealth {
    export interface Props {
        hero: withDullSchema.Infer.Entity<HeroSchema>;
    }
}

export const HeroHealth: FC<HeroHealth.Props> = (
    {
        hero,
    }
) => {
    return <Progress
        value={hero.health}
        color={hero.health > 95 ? "green.8" :
            hero.health > 85 ? "green.6" :
                hero.health > 75 ? "green.4" :
                    hero.health > 65 ? "yellow" :
                        hero.health > 50 ? "orange" :
                            hero.health > 40 ? "red.4" : "red.8"
        }
    />;
};
