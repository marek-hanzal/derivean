import {type Schema}         from "@use-pico/extras";
import {cn}                  from "@use-pico/utils";
import {type HTMLAttributes} from "react";
import {css}                 from "../../tools/css";
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
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "table-header-group",
            "font-bold",
            "relative",
            "shadow-sm shadow-zinc-300",
        ])}
        {...$props}
    >
        <div className={cn([
            "table-row",
            "divide-solid divide-x divide-zinc-300",
        ])}>
            {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
                const {
                    cx,
                    $props
                } = css({});

                return <div
                    key={key}
                    className={cx([
                        "table-cell",
                        "text-left",
                        "py-2 px-4",
                        column.width,
                    ])}
                    {...$props}
                >
                    {column.title}
                </div>;
            })}
        </div>
    </div>;
};
