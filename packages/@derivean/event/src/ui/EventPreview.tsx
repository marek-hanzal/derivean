import {
    BoolInline,
    Group,
    Preview,
    Text
}                      from "@use-pico/client";
import {type Infer}    from "@use-pico/extras";
import {
    t,
    td
}                      from "@use-pico/translator";
import {type FC}       from "react";
import {EventInline}   from "../inline/EventInline";
import {EventSchema}   from "../schema/EventSchema";
import {EventDuration} from "./EventDuration";

export namespace EventPreview {
    export interface Props {
        event: Infer.Entity<EventSchema>;
    }
}

export const EventPreview: FC<EventPreview.Props> = (
    {
        event,
    }
) => {
    return <Preview
        items={[
            {
                label: t()`Event name`,
                value: <Group gap={"xs"}>
                           <EventInline entity={event}/>
                           <Text c={"dimmed"}>
                               ({event.name})
                           </Text>
                       </Group>,
            },
            {
                label: t()`Event type`,
                value: td()(`Event type [${event.type}]`),
            },
            {
                label: t()`Event instant`,
                value: <BoolInline bool={event.instant}/>,
            },
            {
                label: t()`Event duration`,
                value: <EventDuration duration={event.duration}/>,
            },
            {
                label: t()`Event start`,
                value: event.from ? <DateInline date={event.from}/> : "-",
            },
            {
                label: t()`Event end`,
                value: event.to ? <DateInline date={event.to}/> : "-",
            },
        ]}
    />;
};
