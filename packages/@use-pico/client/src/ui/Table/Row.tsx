import {
    type Infer,
    type Schema
}                            from "@use-pico/extras";
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
    return <div className={"table-row"}>
        {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
            const {
                cn,
                $props
            } = css(props);

            return <div
                key={key}
                className={cn([
                    "table-cell",
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
