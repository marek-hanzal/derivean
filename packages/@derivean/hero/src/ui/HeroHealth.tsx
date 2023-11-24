import {
    Progress,
    type WithEntity
}                   from "@use-pico/client";
import {type Infer} from "@use-pico/extras";
import {type FC}    from "react";
import {HeroSchema} from "../schema/HeroSchema";

export namespace HeroHealth {
    export interface Props extends WithEntity<Infer.Entity<HeroSchema>> {
    }
}

export const HeroHealth: FC<HeroHealth.Props> = (
    {
        entity,
    }
) => {
    return <Progress
        size={"xl"}
        value={entity.health}
        color={entity.health > 95 ? "green.8" :
            entity.health > 85 ? "green.6" :
                entity.health > 75 ? "green.4" :
                    entity.health > 65 ? "yellow" :
                        entity.health > 50 ? "orange" :
                            entity.health > 40 ? "red.4" : "red.8"
        }
    />;
};
