import {type WithEntity}  from "@use-pico/client";
import {type Infer}       from "@use-pico/extras";
import {tdv}              from "@use-pico/translator";
import {type FC}          from "react";
import {type EventSchema} from "../schema/EventSchema";

export namespace EventInline {
    export interface Props extends WithEntity<Infer.Entity<EventSchema>> {
    }
}

export const EventInline: FC<EventInline.Props> = (
    {
        entity,
    }
) => {
    return tdv()(`Event [${entity.name}]`);
};
