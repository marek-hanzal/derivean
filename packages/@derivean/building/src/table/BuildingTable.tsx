"use client";

import {
    ProducerInline,
    ProducerUI,
    ProductionTime
}                           from "@derivean/producer";
import {
    BuildingIcon,
    ProducerIcon
}                           from "@derivean/ui";
import {t}                  from "@use-pico/i18n";
import {
    ButtonLink,
    Loader
}                           from "@use-pico/ui";
import {HumanSeconds}       from "@use-pico/ui-extra";
import {
    type ComponentProps,
    type FC
}                           from "react";
import {BuildingUpsertForm} from "../form/BuildingUpsertForm";
import {BuildingInline}     from "../inline/BuildingInline";
import {BuildingRpc}        from "../rpc/BuildingRpc";
import {BuildingUI}         from "../ui/BuildingUI";

export namespace BuildingTable {
    export type Columns =
        | "name"
        | "producer"
        | "time"
        | "pipelineTime";

    export type Props = Omit<
        ComponentProps<typeof BuildingUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <BuildingUI.Table
        text={{
            total: t()`Total count of buildings`,
        }}
        name={"building"}
        icon={<BuildingIcon/>}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create new building`,
                    label: t()`Create building`,
                },
            },
            upsertForm: ({modalId}) => <BuildingUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update building`,
                    label: t()`Update building`,
                },
                delete: {
                    label: t()`Delete building`,
                    modal: {
                        title:   t()`Delete building (modal)`,
                        content: t()`Do you really want to delete selected building?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Building has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: BuildingRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <BuildingUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name:         {
                title:  t()`Building name`,
                render: ({item}) => <ButtonLink
                    icon={<BuildingIcon/>}
                    href={{
                        href:  "/manager/building/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={<BuildingInline entity={item}/>}
                />,
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
