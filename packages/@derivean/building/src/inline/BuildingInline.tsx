import {withDullSchema}      from "@use-pico/dull-stuff";
import {td}                  from "@use-pico/i18n";
import {type WithEntity}     from "@use-pico/types";
import {type FC}             from "react";
import {type BuildingSchema} from "../schema/BuildingSchema";

export namespace BuildingInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<BuildingSchema>> {
    }
}

export const BuildingInline: FC<BuildingInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Building [${entity.name}]`);
};
