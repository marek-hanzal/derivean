import {type Schema}         from "@use-pico/extras";
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
            "shadow-md shadow-zinc-200",
        ])}
        {...$props}
    >
        <div className={"table-row"}>
            {Object.entries<Table.Column<TSchema>>(columns).map(([key, column]) => {
                const {
                    cx,
                    $props
                } = css({});
                return <div
                    key={key}
                    className={cx([
                        "table-cell",
                        "font-bold",
                        "py-2 px-4",
                        "bg-zinc-100",
                    ])}
                    {...$props}
                >
                    {column.title}
                </div>;
            })}
        </div>
    </div>;
};
