import {HumanTime} from "@use-pico/client";
import {type FC}   from "react";

export namespace EventDuration {
    export interface Props {
        duration?: number | null;
    }
}

export const EventDuration: FC<EventDuration.Props> = (
    {
        duration,
    }
) => {
    return duration ? <HumanTime seconds={duration * 60 * 60 * 24}/> : "-";
};
