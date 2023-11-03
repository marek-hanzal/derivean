import {type WithEntity}         from "@use-pico/types";
import {type FC}                 from "react";
import {type ResourceTypeSchema} from "../schema/type/ResourceTypeSchema";

export namespace ResourceTypeInline {
    export interface Props extends WithEntity.Schema<ResourceTypeSchema> {
    }
}

export const ResourceTypeInline: FC<ResourceTypeInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
