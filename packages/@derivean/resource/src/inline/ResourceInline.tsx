import {withDullSchema}      from "@use-pico/dull-stuff";
import {type WithEntity}     from "@use-pico/types";
import {type FC}             from "react";
import {type ResourceSchema} from "../schema/ResourceSchema";

export namespace ResourceInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<ResourceSchema>> {
    }
}

export const ResourceInline: FC<ResourceInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
