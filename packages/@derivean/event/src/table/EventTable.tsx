"use client";

import {
    ProducerInline,
    ProducerUI,
    ProductionTime
}                         from "@derivean/producer";
import {
    EventIcon,
    ProducerIcon
}                         from "@derivean/ui";
import {t}                from "@use-pico/i18n";
import {
    ButtonLink,
    Loader
}                         from "@use-pico/ui";
import {HumanSeconds}     from "@use-pico/ui-extra";
import {
    type ComponentProps,
    type FC
}                         from "react";
import {EventUpsertForm}  from "../form/EventUpsertForm";
import {EventInline}      from "../inline/EventInline";
import {EventRpc}         from "../rpc/EventRpc";
import {EventRequirement} from "../ui/EventRequirement";
import {EventUI}          from "../ui/EventUI";

export namespace EventTable {
    export type Columns =
        | "name"
        | "producer"
        | "time"
        | "requirements"
        | "pipelineTime";

    export type Props = Omit<
        ComponentProps<typeof EventUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const EventTable: FC<EventTable.Props> = props => {
    return <EventUI.Table
        text={{
            total: t()`Total count of events`,
        }}
        name={"event"}
        icon={<EventIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create new event`,
                    label: t()`Create event`,
                },
            },
            upsertForm: ({modalId}) => <EventUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update event`,
                    label: t()`Update event`,
                },
                delete: {
                    label: t()`Delete event`,
                    modal: {
                        title:   t()`Delete event (modal)`,
                        content: t()`Do you really want to delete selected event?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Event has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: EventRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <EventUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name:         {
                title:  t()`Event name`,
                render: ({item}) => <ButtonLink
                    icon={<EventIcon/>}
                    href={{
                        href:  "/manager/event/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={<EventInline entity={item}/>}
                />,
            },
            requirements: {
                title:  t()`Event requirement (label)`,
                render: ({item}) => <EventRequirement
                    eventId={item.id}
                />,
                width:  32,
            },
            producer:     {
                title:  t()`Producer name`,
                render: ({item}) => <ProducerUI.Fetch
                    override={item.producerId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<ProducerIcon/>}
                        href={{
                            href:  "/manager/producer/[id]",
                            query: {
                                id: entity.id,
                            },
                        }}
                        label={<ProducerInline entity={entity}/>}
                    />}
                />,
                width:  18,
            },
            time:         {
                title:  t()`Production time`,
                render: ({item}) => <ProducerUI.Fetch
                    override={item.producerId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <HumanSeconds seconds={entity.time}/>}
                />,
                width:  14,
            },
            pipelineTime: {
                title:  t()`Producer pipeline time`,
                render: ({item}) => <ProducerUI.Fetch
                    override={item.producerId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ProductionTime producerId={entity.id}/>}
                />,
                width:  14,
            },
        }}
        {...props}
    />;
};
