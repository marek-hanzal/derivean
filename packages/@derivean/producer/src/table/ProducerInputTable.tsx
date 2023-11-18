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
import {ProducerInputUpsertForm}  from "../form/ProducerInputUpsertForm";
import {ProducerInline}           from "../inline/ProducerInline";
import {ProducerInputRpc}         from "../rpc/ProducerInputRpc";
import {ProducerInputComponents}  from "../ui/ProducerInputComponents";
import {ProducerOutputComponents} from "../ui/ProducerOutputComponents";
import {ProducerUI}               from "../ui/ProducerUI";

export namespace ProducerInputTable {
    export type Columns =
        | "producerId"
        | "itemId"
        | "producers"
        | "amount";

    export type Props =
        Omit<
            ComponentProps<typeof ProducerInputComponents.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            producerId?: string;
        }
}

export const ProducerInputTable: FC<ProducerInputTable.Props> = (
    {
        producerId,
        ...props
    }
) => {
    return <ProducerInputComponents.Table
        text={{
            total: t()`Total count of producer inputs`,
        }}
        name={"producer.input"}
        icon={<ProducerIcon/>}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create producer input (modal)`,
                    label: t()`Create producer input`,
                },
            },
            upsertForm: ({modalId}) => <ProducerInputUpsertForm
                producerId={producerId}
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update producer input`,
                    label: t()`Update producer input`,
                },
                delete: {
                    label: t()`Delete producer input`,
                    modal: {
                        title:   t()`Delete producer input (modal)`,
                        content: t()`Do you really want to delete selected producer input?`,
                        success: {
                            title: t()`Success`,
                            message: t()`Producer input has been successfully deleted.`,
                        },
                    }
                },
            },
            withMutation: ProducerInputRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ProducerInputUpsertForm
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
            producers:  {
                title:  t()`Item producers`,
                render: ({item}) => <ProducerOutputComponents.Collection
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
                            c={"red.5"}
                        >
                            {t()`Item without producers`}
                        </Text>}
                    </NativeBreadcrumbs>}
                />,
            },
            amount:     {
                title:  t()`Consumed amount`,
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
