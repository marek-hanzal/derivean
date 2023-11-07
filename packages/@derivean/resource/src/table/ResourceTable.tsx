"use client";

import {ResourceIcon}             from "@derivean/ui";
import {tx}                       from "@use-pico/i18n";
import {Table}                    from "@use-pico/ui-extra";
import {type FC}                  from "react";
import {ResourceTypeFetch}        from "../fetch/ResourceTypeFetch";
import {ResourceUpsertForm}       from "../form/ResourceUpsertForm";
import {ResourceTypeInline}       from "../inline/ResourceTypeInline";
import {withResourceMutation}     from "../mutation/withResourceMutation";
import {ResourceQueryStore}       from "../query/ResourceQueryStore";
import {withResourceQuery}        from "../query/withResourceQuery";
import {type ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {ResourceSchema}           from "../schema/ResourceSchema";
import {ResourceSelectionStore}   from "../store/ResourceSelectionStore";

export namespace ResourceTable {
    export type Columns =
        | "name"
        | "typeId";

    export type Props = Omit<
        Table.Props<
            Columns,
            ResourceSchema,
            ResourceQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const ResourceTable: FC<ResourceTable.Props> = props => {
    return <Table
        text={{
            total: tx()`Resource count`,
            count: {
                loading: {
                    title:    tx()`Loading resources`,
                    subtitle: tx()`We're preparing resource list for you...`,
                },
            },
        }}
        name={"resource"}
        icon={<ResourceIcon/>}
        tableActionProps={{
            text: {
                create: {
                    title: tx()`Create new resource`,
                    label: tx()`Create resource`,
                },
            },
            upsertForm: ({modalId}) => <ResourceUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        SelectionStore={ResourceSelectionStore}
        rowActionProps={{
            text: {
                delete: {
                    title: tx()`Delete resource?`,
                    label: tx()`Delete resource`,
                    modal: {
                        content: tx()`Do you really want to delete selected resource?`,
                        success: {
                            title:   tx()`Success`,
                            message: tx()`Resource has been successfully deleted.`,
                        },
                    }
                },
                update: {
                    title: tx()`Update resource`,
                    label: tx()`Update resource`,
                },
            },
            withMutation: withResourceMutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ResourceUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name:   {
                title: tx()`Resource name`,
                render: ({item}) => item.name,
            },
            typeId: {
                title: tx()`Resource type`,
                withFilter: {
                    isFilter: filter => filter?.typeId !== undefined,
                    onFilter: ({
                                   shallowFilter,
                                   item,
                               }) => {
                        shallowFilter({
                            typeId: item.typeId,
                        });
                    },
                    onClear:  ({shallowFilter}) => {
                        shallowFilter({
                            typeId: undefined,
                        });
                    },
                },
                render:     ({item}) => <ResourceTypeFetch
                    override={item.typeId}
                    WithSuccess={ResourceTypeInline}
                />,
                width:      16,
            },
        }}
        withQueryStore={ResourceQueryStore}
        withSourceQuery={withResourceQuery}
        {...props}
    />;
};
