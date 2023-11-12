"use client";

import {ProducerIcon}           from "@derivean/ui";
import {t}                      from "@use-pico/i18n";
import {ButtonLink}             from "@use-pico/ui";
import {HumanSeconds}           from "@use-pico/ui-extra";
import {
    type ComponentProps,
    type FC
}                               from "react";
import {ProducerUpsertForm}     from "../form/ProducerUpsertForm";
import {ProducerInline}         from "../inline/ProducerInline";
import {ProducerRpc}            from "../rpc/ProducerRpc";
import {ProducerSelectionStore} from "../store/ProducerSelectionStore";
import {Dependencies}           from "../ui/Dependencies";
import {ProducerUI}             from "../ui/ProducerUI";
import {ProductionTime}         from "../ui/ProductionTime";

export namespace ProducerTable {
    export type Columns =
        | "name"
        | "dependencies"
        | "pipelineTime"
        | "time";

    export type Props = Omit<
        ComponentProps<typeof ProducerUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const ProducerTable: FC<ProducerTable.Props> = props => {
    return <ProducerUI.Table
        text={{
            total: t()`Producer count`,
        }}
        scrollWidth={2400}
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
            name:         {
                title:  t()`Producer name`,
                render: ({item}) => <ButtonLink
                    icon={<ProducerIcon/>}
                    href={{
                        href:  "/manager/producer/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={<ProducerInline entity={item}/>}
                />,
                width:  14,
            },
            time:         {
                title:  t()`Production time`,
                render: ({item}) => <HumanSeconds seconds={item.time}/>,
                width:  14,
            },
            pipelineTime: {
                title:  t()`Producer pipeline time`,
                render: ({item}) => <ProductionTime producerId={item.id}/>,
                width:  14,
            },
            dependencies: {
                title:  t()`Producer dependencies`,
                render: ({item}) => <Dependencies
                    producerId={item.id}
                />,
            },
        }}
        {...props}
    />;
};
