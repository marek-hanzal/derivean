"use client";

import {EventIcon}       from "@derivean/ui";
import {
    DateInline,
    t,
    td
}                        from "@use-pico/i18n";
import {
    BoolInline,
    ButtonLink
}                        from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                        from "react";
import {EventUpsertForm} from "../form/EventUpsertForm";
import {EventInline}     from "../inline/EventInline";
import {EventRpc}        from "../rpc/EventRpc";
import {EventDuration}   from "../ui/EventDuration";
import {EventUI}         from "../ui/EventUI";

export namespace EventTable {
    export type Columns =
        | "name"
        | "instant"
        | "duration"
        | "from"
        | "type"
        | "to";

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
                    title: t()`Create event (modal)`,
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
            name:     {
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
            type: {
                title:  t()`Event type`,
                render: ({item}) => td()(`Event type [${item.type}]`),
                width:  14,
            },
            instant:  {
                title:  t()`Event instant`,
                render: ({item}) => <BoolInline bool={item.instant}/>,
                width:  14,
            },
            duration: {
                title:  t()`Event duration`,
                render: ({item}) => <EventDuration duration={item.duration}/>,
                width:  14,
            },
            from:     {
                title:  t()`Event start`,
                render: ({item}) => item.from ? <DateInline date={item.from}/> : "-",
                width:  14,
            },
            to:       {
                title:  t()`Event end`,
                render: ({item}) => item.to ? <DateInline date={item.to}/> : "-",
                width:  14,
            },
        }}
        {...props}
    />;
};
