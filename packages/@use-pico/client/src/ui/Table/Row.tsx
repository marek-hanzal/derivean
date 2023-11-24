import {
    type Infer,
    type Schema
}                            from "@use-pico/extras";
import {cn}                  from "@use-pico/utils";
import {type HTMLAttributes} from "react";
import {css}                 from "../../tools/css";
import {Table}               from "./Table";

export namespace Row {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > extends HTMLAttributes<HTMLDivElement> {
        columns: Table.Columns<TColumns, TSchema>;
        entity: Infer.Entity<TSchema>;
    }
}

export const Row = <
    TColumns extends string,
    TSchema extends Schema<any, any, any, any>,
>(
    {
        columns,
        entity,
        ...props
    }: Row.Props<TColumns, TSchema>
) => {
    return <div
        className={cn([
            "table-row",
            "divide-solid divide-x divide-zinc-200",
            "shadow-sm shadow-zinc-300",
            "even:bg-zinc-100",
            "hover:(shadow-md relative)",
        ])}
    >
        {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
            const {
                cx,
                $props
            } = css(props);

            return <div
                key={key}
                className={cx([
                    "table-cell",
                    "vertical-middle",
                    "font-bold",
                    "py-2 px-4",
                ])}
                {...$props}
            >
                {column.render({item: entity})}
            </div>;
        })}
    </div>;
};
