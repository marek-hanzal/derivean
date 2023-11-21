import {
    type Infer,
    type Schema
}                            from "@use-pico/extras";
import {type HTMLAttributes} from "react";
import {tailwindify}         from "../../tools/tailwindify";
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
    return <>
        {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
            const {
                cn,
                $props
            } = tailwindify(props);

            return <div
                key={key}
                className={cn([
                    "font-bold",
                    "py-2 px-4",
                    "bg-zinc-100",
                ])}
                {...$props}
            >
                {column.render({item: entity})}
            </div>;
        })}
    </>;
};
