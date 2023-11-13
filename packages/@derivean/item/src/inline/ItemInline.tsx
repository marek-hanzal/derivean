import {withDullSchema}  from "@use-pico/dull-stuff";
import {td}              from "@use-pico/i18n";
import {type WithEntity} from "@use-pico/types";
import {type FC}         from "react";
import {type ItemSchema} from "../schema/ItemSchema";

export namespace ItemInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<ItemSchema>> {
    }
}

export const ItemInline: FC<ItemInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Item [${entity.name}]`);
};
