import {withDullSchema}   from "@use-pico/dull-stuff";
import {td}               from "@use-pico/i18n";
import {type WithEntity}  from "@use-pico/types";
import {type FC}          from "react";
import {type EventSchema} from "../schema/EventSchema";

export namespace EventInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<EventSchema>> {
    }
}

export const EventInline: FC<EventInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Event [${entity.name}]`);
};
