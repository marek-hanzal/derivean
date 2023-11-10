import {withDullSchema}     from "@use-pico/dull-stuff";
import {type WithEntity}    from "@use-pico/types";
import {type FC}            from "react";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export namespace ResourceTypeInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<ResourceTypeSchema>> {
    }
}

export const ResourceTypeInline: FC<ResourceTypeInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
