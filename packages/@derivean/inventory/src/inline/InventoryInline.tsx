import {withDullSchema}       from "@use-pico/dull-stuff";
import {td}                   from "@use-pico/i18n";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {type InventorySchema} from "../schema/InventorySchema";

export namespace InventoryInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<InventorySchema>> {
    }
}

export const InventoryInline: FC<InventoryInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Inventory [${entity.name}]`);
};
