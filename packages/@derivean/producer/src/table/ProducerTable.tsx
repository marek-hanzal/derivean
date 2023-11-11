"use client";

import {ProducerIcon}           from "@derivean/ui";
import {withDullSchema}         from "@use-pico/dull-stuff";
import {t}                      from "@use-pico/i18n";
import {ButtonLink}             from "@use-pico/ui";
import {
    HumanSeconds,
    Table
}                               from "@use-pico/ui-extra";
import {type FC}                from "react";
import {ProducerUpsertForm}     from "../form/ProducerUpsertForm";
import {ProducerQueryStore}     from "../query/ProducerQueryStore";
import {ProducerRpc}            from "../rpc/ProducerRpc";
import {type ProducerSchema}    from "../schema/ProducerSchema";
import {ProducerSelectionStore} from "../store/ProducerSelectionStore";

export namespace ProducerTable {
    export type Columns =
        | "name"
        | "time";

    export type Props = Omit<
        Table.Props<
            Columns,
            withDullSchema.Infer.EntitySchema<ProducerSchema>,
            withDullSchema.Infer.QuerySchema<ProducerSchema>
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const ProducerTable: FC<ProducerTable.Props> = props => {
    return <Table
        text={{
            total: t()`Producer count`,
        }}
        name={"producer"}
        icon={<ProducerIcon/>}
        SelectionStore={ProducerSelectionStore.single}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create new producer`,
                    label: t()`Create producer`,
                }
            },
            upsertForm: ({modalId}) => <ProducerUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                delete: {
                    label: t()`Delete producer`,
                    modal: {
                        title:   t()`Delete producer (modal)`,
                        content: t()`Do you really want to delete selected producer?`,
                        success: {
                            title: t()`Success`,
                            message: t()`Producer has been successfully deleted.`,
                        },
                    }
                },
                update: {
                    title: t()`Update producer`,
                    label: t()`Update producer`,
                },
            },
            withMutation: ProducerRpc.mutation,
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
                title: t()`Producer name`,
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
                title: t()`Production time`,
                render: ({item}) => <HumanSeconds seconds={item.time}/>,
                width:  14,
            },
        }}
        withQueryStore={ProducerQueryStore}
        withSourceQuery={ProducerRpc.query}
        {...props}
    />;
};
