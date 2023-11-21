import {
    type Infer,
    type Schema
}                              from "@use-pico/extras";
import {
    type FC,
    type ReactNode
}                              from "react";
import {type IQueryStore}      from "../api/IQueryStore";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace Table {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > {
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
        columns: Columns<TColumns, TSchema>;
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
    }: Table.Props<TColumns, TSchema>,
) => {
    return "Table";
};
