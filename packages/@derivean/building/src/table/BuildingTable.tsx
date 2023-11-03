"use client";

import {WithTranslationProvider}  from "@use-pico/i18n";
import {Table}                    from "@use-pico/table";
import {type FC}                  from "react";
import {BuildingQueryStore}       from "../query/BuildingQueryStore";
import {withBuildingQuery}        from "../query/withBuildingQuery";
import {type BuildingQuerySchema} from "../schema/BuildingQuerySchema";
import {type BuildingSchema}      from "../schema/BuildingSchema";
import {BuildingTableAction}      from "./BuildingTable/BuildingTableAction";
import {BuildingTableRowAction}   from "./BuildingTable/BuildingTableRowAction";

export namespace BuildingTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            BuildingSchema,
            BuildingQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <WithTranslationProvider
        withTranslation={{
            namespace: "building",
        }}
    >
        <Table
            WithTableAction={BuildingTableAction}
            WithRowAction={BuildingTableRowAction}
            columns={{
                name: {
                    render: ({item}) => item.name,
                },
            }}
            withQueryStore={BuildingQueryStore}
            withSourceQuery={withBuildingQuery}
            {...props}
        />
    </WithTranslationProvider>;
};
