import {
    type FilterSchema,
    type IWithSourceQuery,
    type OrderBySchema,
    type QuerySchema
}                               from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                               from "@use-pico2/schema";
import {
    type IMultiSelectionStore,
    type ISelectionStore
}                               from "@use-pico2/selection";
import {Table}                  from "@use-pico2/ui";
import {
    classNames,
    generateId,
    isString
}                               from "@use-pico2/utils";
import {
    type FC,
    type PropsWithChildren
}                               from "react";
import {type ITableColumnTuple} from "../../api/ITableColumnTuple";
import classes                  from "../Table.module.css";

export namespace TableBody {
    export interface Props<
        TSchema extends WithIdentitySchema,
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
    > {
        withSourceQuery: IWithSourceQuery<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>;
        SelectionStore?: ISelectionStore<PicoSchema.Output<TSchema>>;
        MultiSelectionStore?: IMultiSelectionStore<PicoSchema.Output<TSchema>>;
        WithRow: FC<RowProps<TSchema>>;
        /**
         * Per-row component action handler
         */
        WithRowAction?: FC<WithRowActionProps<TSchema>>;
        withTableAction: boolean;
        columns: ITableColumnTuple<TSchema, TQuerySchema>[];
        disableActions: boolean;
        highlight?: string[];

        onClick?(item: PicoSchema.Output<TSchema>): void;
    }

    export interface WithRowActionProps<TSchema extends PicoSchema> {
        item: PicoSchema.Output<TSchema>;
    }

    export type RowProps<TSchema extends PicoSchema> = PropsWithChildren<{
        className: string;
        entity: PicoSchema.Output<TSchema>;
    }>;
}

export const TableBody = <
    TSchema extends WithIdentitySchema,
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
>(
    {
        withSourceQuery,
        SelectionStore,
        MultiSelectionStore,
        WithRow,
        WithRowAction,
        withTableAction,
        disableActions,
        columns,
        onClick,
        highlight,
    }: TableBody.Props<TSchema, TFilterSchema, TOrderBySchema, TQuerySchema>
) => {
    const selection = SelectionStore?.use$((
        {
            isSelected,
            isCurrent,
            select,
        }) => ({
        isSelected,
        isCurrent,
        select,
    }));
    const multiSelection = MultiSelectionStore?.use$((
        {
            isSelected,
            isCurrent,
            toggle,
            select,
        }) => ({
        isSelected,
        isCurrent,
        toggle,
        select,
    }));
    const result = withSourceQuery.useQuery();

    return <Table.Tbody>
        {(result.data || [])
            .map(item => <WithRow
                entity={item}
                key={generateId()}
                className={classNames(
                    (selection?.isSelected(item) || multiSelection?.isSelected(item)) ? classes.selection : (selection?.isCurrent(item) || multiSelection?.isCurrent(item) ? classes.active : undefined),
                )}
            >
                {!disableActions && withTableAction && !WithRowAction && <Table.Td/>}
                {!disableActions && WithRowAction && <Table.Td>
                    {WithRowAction && <WithRowAction item={item}/>}
                </Table.Td>}
                {columns.map(([name, {
                    render,
                    withFilter
                }]) => {
                    const Render = render;
                    const children = <Render item={item} highlight={isString(highlight) ? [highlight] : highlight}/>;
                    return <Table.Td
                        key={name}
                        style={(selection || multiSelection || onClick) ? {cursor: "pointer"} : undefined}
                        onClick={() => {
                            selection?.select(item);
                            multiSelection?.toggle(item);
                            onClick?.(item);
                        }}
                    >
                        {children}
                    </Table.Td>;
                })}
            </WithRow>)}
    </Table.Tbody>;
};
