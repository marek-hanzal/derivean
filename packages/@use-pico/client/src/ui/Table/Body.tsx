import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {type HTMLAttributes}   from "react";
import {type IQueryStore}      from "../../api/IQueryStore";
import {type IWithSourceQuery} from "../../api/IWithSourceQuery";
import {css}                   from "../../tools/css";
import {Rows}                  from "./Rows";
import {type Table}            from "./Table";

export namespace Body {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > extends HTMLAttributes<HTMLDivElement> {
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

export const Body = <
    TColumns extends string,
    TSchema extends Schema<any, any, any, any>,
>(
    {
        withQueryStore,
        withSourceQuery,
        columns,
        ...props
    }: Body.Props<TColumns, TSchema>
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "table-row-group",
        ])}
        {...$props}
    >
        <Rows
            columns={columns}
            withQueryStore={withQueryStore}
            withSourceQuery={withSourceQuery}
        />
    </div>;
};
