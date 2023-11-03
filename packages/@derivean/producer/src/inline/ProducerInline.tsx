import {type WithEntity}     from "@use-pico/types";
import {type FC}             from "react";
import {type ProducerSchema} from "../schema/ProducerSchema";

export namespace ProducerInline {
    export interface Props extends WithEntity.Schema<ProducerSchema> {
    }
}

export const ProducerInline: FC<ProducerInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
