import {withDullSchema}  from "@use-pico/dull-stuff";
import {type WithEntity} from "@use-pico/types";
import {type FC}         from "react";
import {type HeroSchema} from "../schema/HeroSchema";

export namespace HeroInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<HeroSchema>> {
    }
}

export const HeroInline: FC<HeroInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
