"use client";

import {
    ProducerInput,
    ProducerOutput,
    ProducerUI,
    ProductionTime
}                         from "@derivean/producer";
import {withDullSchema}   from "@use-pico/dull-stuff";
import {t}                from "@use-pico/i18n";
import {
    Group,
    Loader,
    Preview,
    Text
}                         from "@use-pico/ui";
import {HumanSeconds}     from "@use-pico/ui-extra";
import {type FC}          from "react";
import {EventInline}      from "../inline/EventInline";
import {EventSchema}      from "../schema/EventSchema";
import {EventRequirement} from "./EventRequirement";

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
                label: t()`Production time`,
                value: <ProducerUI.Fetch
                           override={event.producerId}
                           loader={<Loader size={"md"} type={"dots"}/>}
                           WithSuccess={({entity}) => <HumanSeconds seconds={entity.time}/>}
                       />,
            },
            {
                label: t()`Event pipeline time`,
                value: <ProductionTime producerId={event.producerId}/>,
            },
            {
                label: t()`Event requirement (label)`,
                value: <EventRequirement eventId={event.id}/>
            },
            {
                label: t()`Event input`,
                value: <ProducerInput producerId={event.producerId}/>
            },
            {
                label: t()`Event output`,
                value: <ProducerOutput producerId={event.producerId}/>
            },
        ]}
    />;
};
