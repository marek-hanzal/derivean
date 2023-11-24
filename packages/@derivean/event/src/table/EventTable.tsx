"use client";

import {EventIcon}       from "@derivean/ui";
import {
    BoolInline,
    ButtonLink,
    DateInline
}                        from "@use-pico/client";
import {
    t,
    td
}                        from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                        from "react";
import {EventInline}     from "../inline/EventInline";
import {EventComponents} from "../ui/EventComponents";
import {EventDuration}   from "../ui/EventDuration";

export namespace EventTable {
    export type Columns =
        | "name"
        | "instant"
        | "duration"
        | "from"
        | "type"
        | "to";

    export type Props = Omit<
        ComponentProps<typeof EventComponents.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const EventTable: FC<EventTable.Props> = props => {
    return <EventComponents.Table
        text={{
            total: t`Total count of events`,
        }}
        icon={<EventIcon/>}
        // tableActionProps={{
        //     text:       {
        //         create: {
        //             title: t`Create event (modal)`,
        //             label: t`Create event`,
        //         },
        //     },
        //     upsertForm: ({modalId}) => <EventUpsertForm
        //         withAutoClose={[modalId]}
        //     />,
        // }}
        // rowActionProps={{
        //     text:         {
        //         update: {
        //             title: t`Update event`,
        //             label: t`Update event`,
        //         },
        //         delete: {
        //             label: t`Delete event`,
        //             modal: {
        //                 title:   t`Delete event (modal)`,
        //                 content: t`Do you really want to delete selected event?`,
        //                 success: {
        //                     title:   t`Success`,
        //                     message: t`Event has been successfully removed`,
        //                 },
        //             },
        //         },
        //     },
        //     withMutation: EventRpc.mutation,
        //     upsertForm:   ({
        //                        item,
        //                        modalId
        //                    }) => <EventUpsertForm
        //         withAutoClose={[modalId]}
        //         entity={item}
        //     />,
        // }}
        columns={{
            name:     {
                title:  t`Event name`,
                render: ({item}) => <ButtonLink
                    icon={<EventIcon/>}
                    href={{
                        href:  "/manager/event/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                >
                    <EventInline entity={item}/>
                </ButtonLink>,
            },
            type:     {
                title:  t`Event type`,
                render: ({item}) => td(`Event type [${item.type}]`),
                width:  "w-54",
            },
            instant:  {
                title:  t`Event instant`,
                render: ({item}) => <div className={"flex items-center"}>
                    <BoolInline cn={["grow"]} bool={item.instant}/>
                </div>,
                width:  "w-54",
            },
            duration: {
                title: t`Event duration`,
                render: ({item}) => <EventDuration duration={item.duration}/>,
                width: "w-54",
            },
            from:     {
                title: t`Event start`,
                render: ({item}) => item.from ? <DateInline date={item.from}/> : "-",
                width: "w-42",
            },
            to:       {
                title: t`Event end`,
                render: ({item}) => item.to ? <DateInline date={item.to}/> : "-",
                width: "w-42",
            },
        }}
        {...props}
    />;
};
