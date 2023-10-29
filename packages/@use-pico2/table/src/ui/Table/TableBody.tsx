import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                               from "@use-pico2/query";
import {type PicoSchema}        from "@use-pico2/schema";
import {
    type IMultiSelectionStore,
    type ISelectionStore
}                               from "@use-pico2/selection";
import {
    type FC,
    type PropsWithChildren
}                               from "react";
import {type ITableColumnTuple} from "../../api/ITableColumnTuple";

export namespace TableBody {
    export interface Props<
        TSchema extends PicoSchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > {
        // withSourceQuery: WithSourceQuery<TSchema, TQuerySchema>;
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
    TSchema extends PicoSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
>(
    {
        // withSourceQuery,
        SelectionStore,
        MultiSelectionStore,
        WithRow,
        WithRowAction,
        withTableAction,
        disableActions,
        columns,
        onClick,
        highlight,
    }: TableBody.Props<TSchema, TQuerySchema>
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
    // const result = withSourceQuery.useQuery();

    return "TableBody";

    // return <Table.Tbody>
    //     {(result.data || [])
    //         .map(item => <WithRow
    //             entity={item}
    //             key={generateId()}
    //             className={classNames(
    //                 (selection?.isSelected(item) || multiSelection?.isSelected(item)) ? classes.selection : (selection?.isCurrent(item) || multiSelection?.isCurrent(item) ? classes.active : undefined),
    //             )}
    //         >
    //             {!disableActions && withTableAction && !WithRowAction && <Table.Td/>}
    //             {!disableActions && WithRowAction && <Table.Td>
    //                 {WithRowAction && <WithRowAction item={item}/>}
    //             </Table.Td>}
    //             {columns.map(([name, {
    //                 render,
    //                 withFilter
    //             }]) => {
    //                 const Render = render;
    //                 const children = <Render item={item} highlight={isString(highlight) ? [highlight] : highlight}/>;
    //                 return <Table.Td
    //                     key={name}
    //                     style={(selection || multiSelection || onClick) ? {cursor: "pointer"} : undefined}
    //                     onClick={() => {
    //                         selection?.select(item);
    //                         multiSelection?.toggle(item);
    //                         onClick?.(item);
    //                     }}
    //                 >
    //                     {withFilter ? <withSourceQuery.WithFilter
    //                         Filter={({
    //                                      setFilter,
    //                                      shallowFilter,
    //                                      filter,
    //                                      clearFilter,
    //                                  }) => <FilterAction
    //                             isFilter={() => withFilter?.isFilter(filter)}
    //                             onFilter={() => withFilter?.onFilter({
    //                                 setFilter,
    //                                 shallowFilter,
    //                                 filter,
    //                                 item,
    //                                 clearFilter,
    //                             })}
    //                             onClear={() => withFilter?.onClear({
    //                                 setFilter,
    //                                 shallowFilter,
    //                                 filter,
    //                                 item,
    //                                 clearFilter,
    //                             })}
    //                         >
    //                             <div>
    //                                 {children}
    //                             </div>
    //                         </FilterAction>}
    //                     /> : children}
    //                 </Table.Td>;
    //             })}
    //         </WithRow>)}
    // </Table.Tbody>;
};
