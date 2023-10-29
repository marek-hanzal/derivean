import {WithTranslationProvider}  from "@use-pico2/i18n";
import {Table}                    from "@use-pico2/table";
import {type FC}                  from "react";
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
        "columns" | "withSourceQuery"
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
            // withSourceQuery={withResourceQuery}
            {...props}
        />
    </WithTranslationProvider>;
};
