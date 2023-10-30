import {WithTranslationProvider}  from "@use-pico2/i18n";
import {Table}                    from "@use-pico2/table";
import {type FC}                  from "react";
import {ResourceQueryStore}       from "../query/ResourceQueryStore";
import {withResourceQuery}        from "../query/withResourceQuery";
import {type ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {type ResourceSchema}      from "../schema/ResourceSchema";

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
