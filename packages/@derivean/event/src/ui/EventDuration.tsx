import {HumanSeconds} from "@use-pico/ui-extra";
import {type FC}      from "react";

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
    return duration ? <HumanSeconds seconds={duration * 60 * 60 * 24}/> : "-";
};
