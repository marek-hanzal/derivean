import {type WithEntity} from "@use-pico/client";
import {type Infer}      from "@use-pico/extras";
import {tdv}             from "@use-pico/translator";
import {type FC}         from "react";
import {type ItemSchema} from "../schema/ItemSchema";

export namespace ItemInline {
    export interface Props extends WithEntity<Infer.Entity<ItemSchema>> {
    }
}

export const ItemInline: FC<ItemInline.Props> = (
    {
        entity,
    }
) => {
    return tdv()(`Item [${entity.name}]`);
};
