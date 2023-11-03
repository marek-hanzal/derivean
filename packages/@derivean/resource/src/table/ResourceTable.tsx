"use client";

import {ResourceIcon}             from "@derivean/ui";
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
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon"
    >
}

export const ResourceTable: FC<ResourceTable.Props> = props => {
    return <Table
        name={"resource"}
        icon={<ResourceIcon/>}
        tableActionProps={{
            upsertForm: ({modalId}) => <ResourceUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        SelectionStore={ResourceSelectionStore}
        rowActionProps={{
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
                render: ({item}) => item.name,
            },
            typeId: {
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
