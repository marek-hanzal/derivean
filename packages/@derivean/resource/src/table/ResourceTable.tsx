"use client";

import {WithTranslationProvider}  from "@use-pico/i18n";
import {Table}                    from "@use-pico/table";
import {type FC}                  from "react";
import {ResourceQueryStore}       from "../query/ResourceQueryStore";
import {withResourceQuery}        from "../query/withResourceQuery";
import {type ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {type ResourceSchema}      from "../schema/ResourceSchema";
import {ResourceTableAction}      from "./ResourceTable/ResourceTableAction";
import {ResourceTableRowAction}   from "./ResourceTable/ResourceTableRowAction";

export namespace ResourceTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            ResourceSchema,
            ResourceQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore"
    >
}

export const ResourceTable: FC<ResourceTable.Props> = props => {
    return <WithTranslationProvider
        withTranslation={{
            namespace: "resource",
        }}
    >
        <Table
            WithTableAction={ResourceTableAction}
            WithRowAction={ResourceTableRowAction}
            columns={{
                name: {
                    render: ({item}) => item.name,
                },
            }}
            withQueryStore={ResourceQueryStore}
            withSourceQuery={withResourceQuery}
            {...props}
        />
    </WithTranslationProvider>;
};
