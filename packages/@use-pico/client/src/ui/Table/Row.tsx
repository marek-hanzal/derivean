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
            ":uno: table-row divide-solid divide-x divide-zinc-200 shadow-sm shadow-zinc-300 odd:bg-zinc-100 hover:(odd:bg-sky-100 even:bg-sky-100)",
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
                    ":uno: table-cell vertical-middle font-bold py-2 px-2",
                ])}
                {...$props}
            >
                {column.render({item: entity})}
            </div>;
        })}
    </div>;
};
