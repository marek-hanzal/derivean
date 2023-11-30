import {
    BoolInline,
    DateInline,
    Preview,
    Text,
    type WithEntity
}                      from "@use-pico/client";
import {type Infer}    from "@use-pico/extras";
import {
    t,
    tdv
}                      from "@use-pico/translator";
import {type FC}       from "react";
import {EventInline}   from "../inline/EventInline";
import {EventSchema}   from "../schema/EventSchema";
import {EventDuration} from "./EventDuration";

export namespace EventPreview {
    export interface Props extends WithEntity<Infer.Entity<EventSchema>> {
    }
}

export const EventPreview: FC<EventPreview.Props> = (
    {
        entity,
    }
) => {
    return <Preview
        items={[
            {
                key: "name",
                label: t`Event name`,
                value: <div className={"flex gap-4"}>
                           <EventInline entity={entity}/>
                           <Text c={"dimmed"}>
                               ({entity.name})
                           </Text>
                       </div>,
            },
            {
                key: "type",
                label: t`Event type`,
                value: tdv()(`Event type [${entity.type}]`),
            },
            {
                key: "instant",
                label: t`Event instant`,
                value: <BoolInline bool={entity.instant}/>,
            },
            {
                key: "duration",
                label: t`Event duration`,
                value: <EventDuration duration={entity.duration}/>,
            },
            {
                key: "start",
                label: t`Event start`,
                value: entity.from ? <DateInline date={entity.from}/> : "-",
            },
            {
                key: "end",
                label: t`Event end`,
                value: entity.to ? <DateInline date={entity.to}/> : "-",
            },
        ]}
    />;
};
