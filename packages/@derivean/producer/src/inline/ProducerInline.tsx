import {type WithEntity}     from "@use-pico/client";
import {type Infer}          from "@use-pico/extras";
import {td}                  from "@use-pico/translator";
import {type FC}             from "react";
import {type ProducerSchema} from "../schema/ProducerSchema";

export namespace ProducerInline {
    export interface Props extends WithEntity<Infer.Entity<ProducerSchema>> {
    }
}

export const ProducerInline: FC<ProducerInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Producer [${entity.name}]`);
};
