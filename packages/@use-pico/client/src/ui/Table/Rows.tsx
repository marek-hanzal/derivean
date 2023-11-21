import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {type IQueryStore}      from "../../api/IQueryStore";
import {type IWithSourceQuery} from "../../api/IWithSourceQuery";
import {useSourceQuery}        from "../../hook/useSourceQuery";
import {Row}                   from "./Row";
import {Table}                 from "./Table";

export namespace Rows {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > {
        columns: Table.Columns<TColumns, TSchema>;
        withQueryStore: IQueryStore.Store<
            Infer.QuerySchema<TSchema>
        >;
        withSourceQuery: IWithSourceQuery<
            Infer.QuerySchema<TSchema>,
            Infer.EntitySchema<TSchema>
        >;
    }
}

export const Rows = <
    TColumns extends string,
    TSchema extends Schema<any, any, any, any>,
>(
    {
        columns,
        withQueryStore,
        withSourceQuery,
    }: Rows.Props<TColumns, TSchema>
) => {
    const result = useSourceQuery({
        withSourceQuery,
        store: withQueryStore,
    });

    return <>
        {result.data?.map((item) => <Row
            key={item.id}
            columns={columns}
            entity={item}
        />)}
    </>;
};
