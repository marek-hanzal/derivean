"use client";

import {
    ItemInline,
    ItemUI
}                                 from "@derivean/item";
import {ProducerIcon}             from "@derivean/ui";
import {t}                        from "@use-pico/i18n";
import {
    ButtonLink,
    Group,
    NativeBreadcrumbs,
    Text
}                                 from "@use-pico/ui";
import {HumanSeconds}             from "@use-pico/ui-extra";
import {
    type ComponentProps,
    type FC
}                                 from "react";
import {ProducerOutputUpsertForm} from "../form/ProducerOutputUpsertForm";
import {ProducerInline}           from "../inline/ProducerInline";
import {ProducerOutputRpc}        from "../rpc/ProducerOutputRpc";
import {ProducerInputUI}          from "../ui/ProducerInputUI";
import {ProducerOutputUI}         from "../ui/ProducerOutputUI";
import {ProducerUI}               from "../ui/ProducerUI";

export namespace ProducerOutputTable {
    export type Columns =
        | "producerId"
        | "itemId"
        | "consumers"
        | "amount";

    export type Props =
        Omit<
            ComponentProps<typeof ProducerOutputUI.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            producerId?: string;
        }
}

export const ProducerOutputTable: FC<ProducerOutputTable.Props> = (
    {
        producerId,
        ...props
    }
) => {
    return <ProducerOutputUI.Table
        text={{
            total: t()`Total count of producer outputs`,
        }}
        name={"producer.output"}
        icon={<ProducerIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create producer output (modal)`,
                    label: t()`Create producer output`,
                },
            },
            upsertForm: ({modalId}) => <ProducerOutputUpsertForm
                producerId={producerId}
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update producer output`,
                    label: t()`Update producer output`,
                },
                delete: {
                    label: t()`Delete producer output`,
                    modal: {
                        title:   t()`Delete producer output (modal)`,
                        content: t()`Do you really want to delete selected producer output?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Producer output has been successfully deleted.`,
                        },
                    }
                },
            },
            withMutation: ProducerOutputRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ProducerOutputUpsertForm
                producerId={producerId}
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            producerId: {
                title:  t()`Producer name`,
                render: ({item}) => <ProducerUI.Fetch
                    override={item.producerId}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<ProducerIcon/>}
                        href={{
                            href:  "/manager/producer/[id]",
                            query: {
                                id: entity.id,
                            },
                        }}
                        label={entity.name}
                    />}
                />,
            },
            itemId:     {
                title:  t()`Item name`,
                render: ({item}) => <ItemUI.Fetch
                    override={item.itemId}
                    WithSuccess={({entity}) => <ItemInline entity={entity}/>}
                />,
                width: 12,
            },
            consumers:  {
                title:  t()`Item consumers`,
                render: ({item}) => <ProducerInputUI.Collection
                    query={{
                        where: {
                            itemId: item.itemId,
                        }
                    }}
                    WithSuccess={({entities}) => <NativeBreadcrumbs>
                        {entities.map(entity => <ProducerUI.Fetch
                            key={entity.id}
                            override={entity.producerId}
                            WithSuccess={({entity}) => <ButtonLink
                                icon={<ProducerIcon/>}
                                href={{
                                    href: "/manager/producer/[id]/pipeline",
                                    query: {
                                        id: entity.id,
                                    },
                                }}
                                label={<ProducerInline entity={entity}/>}
                            />}
                        />)}
                        {!entities.length && <Text
                            fw={"bold"}
                            c={"gray.6"}
                        >
                            {t()`Item without consumers`}
                        </Text>}
                    </NativeBreadcrumbs>}
                />,
            },
            amount:     {
                title:  t()`Produced amount`,
                render: ({item}) => <ProducerUI.Fetch
                    override={item.producerId}
                    WithSuccess={({entity}) => <Group gap={4}>
                        <Text fw={"bold"}>
                            {item.amount}
                        </Text>
                        /
                        <Text>
                            <HumanSeconds seconds={entity.time}/>
                        </Text>
                    </Group>}
                />,
                width:  14,
            },
        }}
        {...props}
    />;
};
