"use client";

import {ResourceIcon}             from "@derivean/ui";
import {Table}                    from "@use-pico/ui-extra";
import {type FC}                  from "react";
import {ResourceUpsertForm}       from "../form/ResourceUpsertForm";
import {withResourceMutation}     from "../mutation/withResourceMutation";
import {ResourceQueryStore}       from "../query/ResourceQueryStore";
import {withResourceQuery}        from "../query/withResourceQuery";
import {type ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {ResourceSchema}           from "../schema/ResourceSchema";

export namespace ResourceTable {
    export type Columns =
        | "name";

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
            name: {
                render: ({item}) => item.name,
            },
        }}
        withQueryStore={ResourceQueryStore}
        withSourceQuery={withResourceQuery}
        {...props}
    />;
};
