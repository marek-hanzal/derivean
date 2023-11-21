import {
    BoolInline,
    DateInline,
    Group,
    Preview,
    Text,
    type WithEntity
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
                label: t`Event name`,
                value: <Group gap={"xs"}>
                           <EventInline entity={entity}/>
                           <Text c={"dimmed"}>
                               ({entity.name})
                           </Text>
                       </Group>,
            },
            {
                label: t`Event type`,
                value: td()(`Event type [${entity.type}]`),
            },
            {
                label: t`Event instant`,
                value: <BoolInline bool={entity.instant}/>,
            },
            {
                label: t`Event duration`,
                value: <EventDuration duration={entity.duration}/>,
            },
            {
                label: t`Event start`,
                value: entity.from ? <DateInline date={entity.from}/> : "-",
            },
            {
                label: t`Event end`,
                value: entity.to ? <DateInline date={entity.to}/> : "-",
            },
        ]}
    />;
};
