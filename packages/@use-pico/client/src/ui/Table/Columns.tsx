import {type Schema}         from "@use-pico/extras";
import {type HTMLAttributes} from "react";
import {tailwindify}         from "../../tools/tailwindify";
import {type Table}          from "./Table";

export namespace Columns {
    export interface Props<
        TColumns extends string,
        TSchema extends Schema<any, any, any, any>,
    > extends HTMLAttributes<HTMLDivElement> {
        columns: Table.Columns<TColumns, TSchema>;
    }
}

export const Columns = <
    TColumns extends string,
    TSchema extends Schema<any, any, any, any>,
>(
    {
        columns,
        ...props
    }: Columns.Props<TColumns, TSchema>
) => {
    const {
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([
            "flex",
            "divide-solid divide-x divide-zinc-300",
        ])}
        {...$props}
    >
        {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
            const {
                cn,
                $props
            } = tailwindify({});
            return <div
                key={key}
                className={cn([
                    "flex-auto",
                    "font-bold",
                    "py-2 px-4",
                    "bg-zinc-100",
                ])}
                {...$props}
            >
                {column.title}
            </div>;
        })}
    </div>;
};
