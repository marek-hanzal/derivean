"use client";

import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {
    type FC,
    HTMLAttributes,
    type ReactNode
}                              from "react";
import {type IQueryStore}      from "../../api/IQueryStore";
import {type IWithSourceQuery} from "../../api/IWithSourceQuery";
import {useSourceQuery}        from "../../hook/useSourceQuery";
import {css}                   from "../../tools/css";
import {Body}                  from "./Body";
import {Columns}               from "./Columns";
import {Loader}                from "./Loader";

export namespace Table {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > extends HTMLAttributes<HTMLDivElement>, css.Style {
        columns: Columns<TColumns, TSchema>;
        withQueryStore: IQueryStore.Store<
            Infer.QuerySchema<TSchema>
        >;
        withSourceQuery: IWithSourceQuery<
            Infer.QuerySchema<TSchema>,
            Infer.EntitySchema<TSchema>
        >;
        text?: {
            total?: ReactNode;
            empty?: {
                title?: ReactNode;
                message?: ReactNode;
            }
        };
        compact?: boolean;
        icon?: ReactNode;
    }

    export type Columns<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > = Record<
        TColumns,
        Column<TSchema>
    >;

    export interface Column<
        TSchema extends Schema<any, any, any, any>,
    > {
        title?: ReactNode;
        render: FC<Column.RenderProps<TSchema>>;
        /**
         * css class name of column width
         */
        width?: string;
    }

    export namespace Column {
        export interface RenderProps<
            TSchema extends Schema<any, any, any, any>,
        > {
            item: Infer.Entity<TSchema>;
        }
    }
}

export const Table = <
    TColumns extends string,
    TSchema extends Schema<any, any, any, any>,
>(
    {
        withQueryStore,
        withSourceQuery,
        columns,
        text,
        icon,
        compact = false,
        ...props
    }: Table.Props<TColumns, TSchema>,
) => {
    const {
        cx,
        $props
    } = css(props);

    const result = useSourceQuery({
        withSourceQuery,
        store: withQueryStore,
    });

    return <>
        <div
            className={cx([
                "table w-full",
                "border border-zinc-300",
                "shadow-sm shadow-zinc-300",
            ])}
            {...$props}
        >
            <Columns
                columns={columns}
            />
            <Body
                columns={columns}
                withQueryStore={withQueryStore}
                withSourceQuery={withSourceQuery}
            />
        </div>
        {result.isLoading && <Loader/>}
    </>;
};
