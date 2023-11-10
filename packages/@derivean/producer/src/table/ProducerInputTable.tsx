"use client";

import {
    ResourceFetch,
    ResourceInline
}                                  from "@derivean/resource";
import {ProducerIcon}              from "@derivean/ui";
import {t}                         from "@use-pico/i18n";
import {ButtonLink}                from "@use-pico/ui";
import {Table}                     from "@use-pico/ui-extra";
import {type FC}                   from "react";
import {ProducerFetch}             from "../fetch/ProducerFetch";
import {ProducerInputUpsertForm}   from "../form/ProducerInputUpsertForm";
import {withProducerInputMutation} from "../mutation/withProducerInputMutation";
import {ProducerInputQueryStore}   from "../query/input/ProducerInputQueryStore";
import {withProducerInputQuery}    from "../query/input/withProducerInputQuery";
import {ProducerInputQuerySchema}  from "../schema/input/ProducerInputQuerySchema";
import {ProducerInputSchema}       from "../schema/input/ProducerInputSchema";

export namespace ProducerInputTable {
    export type Columns =
        | "producerId"
        | "resourceId"
        | "amount";

    export type Props =
        Omit<
            Table.Props<
                Columns,
                ProducerInputSchema,
                ProducerInputQuerySchema
            >,
            "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
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
    return <Table
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
            text: {
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
            withMutation: withProducerInputMutation,
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
                title: t()`Producer name`,
                render: ({item}) => <ProducerFetch
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
            resourceId: {
                title:  t()`Resource name`,
                render: ({item}) => <ResourceFetch
                    override={item.resourceId}
                    WithSuccess={({entity}) => <ResourceInline entity={entity}/>}
                />,
            },
            amount:     {
                title: t()`Consumed amount`,
                render: ({item}) => `${item.amount} - toHuman()`,
                width:  14,
            },
        }}
        withQueryStore={ProducerInputQueryStore}
        withSourceQuery={withProducerInputQuery}
        {...props}
    />;
};
