import {type WithEntity}     from "@use-pico/client";
import {type Infer}          from "@use-pico/extras";
import {td}                  from "@use-pico/translator";
import {type FC}             from "react";
import {type BuildingSchema} from "../schema/BuildingSchema";

export namespace BuildingInline {
    export interface Props extends WithEntity<Infer.Entity<BuildingSchema>> {
    }
}

export const BuildingInline: FC<BuildingInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Building [${entity.name}]`);
};
