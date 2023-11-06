"use client";

import {ProducerIcon}           from "@derivean/ui";
import {tx}                     from "@use-pico/i18n";
import {ButtonLink}             from "@use-pico/ui";
import {
    HumanSeconds,
    Table
}                               from "@use-pico/ui-extra";
import {type FC}                from "react";
import {ProducerUpsertForm}     from "../form/ProducerUpsertForm";
import {withProducerMutation}   from "../mutation/withProducerMutation";
import {ProducerQueryStore}     from "../query/ProducerQueryStore";
import {withProducerQuery}      from "../query/withProducerQuery";
import {ProducerQuerySchema}    from "../schema/ProducerQuerySchema";
import {type ProducerSchema}    from "../schema/ProducerSchema";
import {ProducerSelectionStore} from "../store/ProducerSelectionStore";

export namespace ProducerTable {
    export type Columns =
        | "name"
        | "time";

    export type Props = Omit<
        Table.Props<
            Columns,
            ProducerSchema,
            ProducerQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const ProducerTable: FC<ProducerTable.Props> = props => {
    return <Table
        label={{
            total: tx()`Producer count`,
        }}
        name={"producer"}
        icon={<ProducerIcon/>}
        SelectionStore={ProducerSelectionStore}
        tableActionProps={{
            upsertForm: ({modalId}) => <ProducerUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            label: {
                deleteModal: {
                    content: tx()`Do you really want to delete selected producer?`,
                    success: {
                        title:   tx()`Success`,
                        message: tx()`Producer has been successfully deleted.`,
                    },
                },
            },
            withMutation: withProducerMutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ProducerUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                render: ({item}) => <ButtonLink
                    icon={<ProducerIcon/>}
                    href={{
                        href:  "/manager/producer/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={item.name}
                />,
            },
            time: {
                render: ({item}) => <HumanSeconds seconds={item.time}/>,
                width:  14,
            },
        }}
        withQueryStore={ProducerQueryStore}
        withSourceQuery={withProducerQuery}
        {...props}
    />;
};
