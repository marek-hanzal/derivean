"use client";

import {BuildingIcon}             from "@derivean/ui";
import {Table}                    from "@use-pico/ui-extra";
import {type FC}                  from "react";
import {BuildingUpsertForm}       from "../form/BuildingUpsertForm";
import {withBuildingMutation}     from "../mutation/withBuildingMutation";
import {BuildingQueryStore}       from "../query/BuildingQueryStore";
import {withBuildingQuery}        from "../query/withBuildingQuery";
import {type BuildingQuerySchema} from "../schema/BuildingQuerySchema";
import {type BuildingSchema}      from "../schema/BuildingSchema";

export namespace BuildingTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            BuildingSchema,
            BuildingQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <Table
        name={"building"}
        icon={<BuildingIcon/>}
        tableActionProps={{
            upsertForm: ({modalId}) => <BuildingUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            withMutation: withBuildingMutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <BuildingUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                render: ({item}) => item.name,
            },
        }}
        withQueryStore={BuildingQueryStore}
        withSourceQuery={withBuildingQuery}
        {...props}
    />;
};
