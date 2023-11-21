"use client";

import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {
    type FC,
    type ReactNode
}                              from "react";
import {type CommonProps}      from "../../api/CommonProps";
import {type IQueryStore}      from "../../api/IQueryStore";
import {type IWithSourceQuery} from "../../api/IWithSourceQuery";
import {tailwindify}           from "../../tools/tailwindify";
import {Body}                  from "./Body";
import {Columns}               from "./Columns";

export namespace Table {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > extends CommonProps {
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
        width?: number;
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
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([
            "table w-full",
            "border border-zinc-300",
            "divide-solid divide-y divide-zinc-300",
        ])}
    >
        <Columns
            columns={columns}
        />
        <Body
            columns={columns}
            withQueryStore={withQueryStore}
            withSourceQuery={withSourceQuery}
        />
    </div>;
};
