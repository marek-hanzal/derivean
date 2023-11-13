import {
    type ValuesSchema,
    withSourceQueryInput
}                               from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                               from "react";
import {ItemTypeInline}         from "../inline/ItemTypeInline";
import {ItemTypeQueryStore}     from "../query/ItemTypeQueryStore";
import {ItemTypeRpc}            from "../rpc/ItemTypeRpc";
import {ItemTypeSelectionStore} from "../store/ItemTypeSelectionStore";
import {ItemTypeTable}          from "../table/ItemTypeTable";

const ItemTypeQueryInput = withSourceQueryInput({
    withQueryStore:  ItemTypeQueryStore,
    withSourceQuery: ItemTypeRpc.query,
    SelectionStore:  ItemTypeSelectionStore.single,
});
type ItemTypeQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof ItemTypeQueryInput<TValuesSchema>;

export namespace ItemTypeSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ItemTypeQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ItemTypeTable>>;
    }
}

export const ItemTypeSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ItemTypeSelect.Props<TValuesSchema>
) => {
    return <ItemTypeQueryInput
        Selector={useCallback(() => <ItemTypeTable withLinkLock {...tableProps}/>, [])}
        Item={ItemTypeInline}
        {...props}
    />;
};
