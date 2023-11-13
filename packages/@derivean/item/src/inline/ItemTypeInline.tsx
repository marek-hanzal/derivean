import {withDullSchema}  from "@use-pico/dull-stuff";
import {type WithEntity} from "@use-pico/types";
import {type FC}         from "react";
import {ItemTypeSchema}  from "../schema/ItemTypeSchema";

export namespace ItemTypeInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<ItemTypeSchema>> {
    }
}

export const ItemTypeInline: FC<ItemTypeInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
