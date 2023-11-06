"use client";

import {
    ResourceFetch,
    ResourceInline
}                                  from "@derivean/resource";
import {ProducerIcon,}             from "@derivean/ui";
import {tx}                        from "@use-pico/i18n";
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
            "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "label"
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
        label={{
            total: tx()`Total count of producer inputs`,
        }}
        name={"producer.input"}
        icon={<ProducerIcon/>}
        tableActionProps={{
            upsertForm: ({modalId}) => <ProducerInputUpsertForm
                producerId={producerId}
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            label: {
                deleteModal: {
                    content: tx()`Do you really want to delete selected producer input?`,
                    success: {
                        title:   tx()`Success`,
                        message: tx()`Producer input has been successfully deleted.`,
                    },
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
                render: ({item}) => <ResourceFetch
                    override={item.resourceId}
                    WithSuccess={ResourceInline}
                />,
            },
            amount:     {
                render: ({item}) => `${item.amount} - toHuman()`,
                width:  14,
            },
        }}
        withQueryStore={ProducerInputQueryStore}
        withSourceQuery={withProducerInputQuery}
        {...props}
    />;
};
