import {type WithEntity} from "@use-pico/client";
import {type Infer}      from "@use-pico/extras";
import {type FC}         from "react";
import {type HeroSchema} from "../schema/HeroSchema";

export namespace HeroInline {
    export interface Props extends WithEntity<Infer.Entity<HeroSchema>> {
    }
}

export const HeroInline: FC<HeroInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
