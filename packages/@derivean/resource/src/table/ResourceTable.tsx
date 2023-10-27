import {WithTranslationProvider} from "@use-pico/i18n";
import {Table}                   from "@use-pico/table";
import {type FC}                 from "react";
import {withResourceQuery}       from "../query/withResourceQuery";
import {ResourceSourceSchema}    from "../schema/ResourceSourceSchema";

export namespace ResourceTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            ResourceSourceSchema["shape"]["entity"],
            ResourceSourceSchema["shape"]["filter"],
            ResourceSourceSchema["shape"]["orderBy"]
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
            withSourceQuery={withResourceQuery}
            {...props}
        />
    </WithTranslationProvider>;
};
