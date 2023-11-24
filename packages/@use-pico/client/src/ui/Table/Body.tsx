import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {type HTMLAttributes}   from "react";
import {type IQueryStore}      from "../../api/IQueryStore";
import {type IWithSourceQuery} from "../../api/IWithSourceQuery";
import {useSourceQuery}        from "../../hook/useSourceQuery";
import {css}                   from "../../tools/css";
import {Loader}                from "./Loader";
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

    const result = useSourceQuery({
        withSourceQuery,
        store: withQueryStore,
    });

    return <div
        className={cx([
            "table-row-group",
        ])}
        {...$props}
    >
        {/*{result.isLoading && <Loader/>}*/}
        <Loader/>
        {/*<Rows*/}
        {/*    columns={columns}*/}
        {/*    withQueryStore={withQueryStore}*/}
        {/*    withSourceQuery={withSourceQuery}*/}
        {/*/>*/}
    </div>;
};
