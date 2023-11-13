"use client";

import {withDullSchema} from "@use-pico/dull-stuff";
import {
    DateInline,
    t,
    td
}                       from "@use-pico/i18n";
import {
    BoolInline,
    Group,
    Preview,
    Text
}                       from "@use-pico/ui";
import {type FC}        from "react";
import {EventInline}    from "../inline/EventInline";
import {EventSchema}    from "../schema/EventSchema";
import {EventDuration}  from "./EventDuration";

export namespace EventPreview {
    export interface Props {
        event: withDullSchema.Infer.Entity<EventSchema>;
    }
}

export const EventPreview: FC<EventPreview.Props> = (
    {
        event,
    }
) => {
    return <Preview
        cols={3}
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
